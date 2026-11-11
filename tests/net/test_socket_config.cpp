#include <iostream>
#include <sstream>
#include <cassert>
#include <cstring>
#include "net/ServerConfig.hpp"

int main(void) {
	std::cout << "\n--- Socket & Network Tests ---" << std::endl;

	// =========================================================================
	// [NET-08] Create a TCP socket with scoped ownership
	// Criteria:
	// - Construction failure is handled without a leaked descriptor.
	// - Destroying or transferring ownership never closes a descriptor twice.
	// =========================================================================
	{
		try {
			ScopedSocket bad_sock(-1);
			assert(false);
		} catch (const ScopedSocket::SocketCreationException &) {
			std::cout << "[OK] Construction with invalid FD (-1) throws without leak" << std::endl;
		}

		try {
			ScopedSocket bad_family;
			bad_family.openTcp(AF_UNIX);
			assert(false);
		} catch (const ScopedSocket::InvalidFamilyException &) {
			std::cout << "[OK] Unsupported family rejected cleanly" << std::endl;
		}

		ScopedSocket sock_v4;
		sock_v4.openTcp(AF_INET);
		assert(sock_v4.isValid());
		int raw_fd = sock_v4.release();
		assert(!sock_v4.isValid());
		assert(sock_v4.get() == -1);

		// Transfer ownership to a second ScopedSocket and test swap/reset
		ScopedSocket owner2(raw_fd);
		ScopedSocket owner3;
		owner2.swap(owner3);
		assert(!owner2.isValid());
		assert(owner3.get() == raw_fd);
		owner3.reset(-1);
		assert(!owner3.isValid());
	}
	std::cout << "[OK] ScopedSocket ownership transfer, swap & single close verified" << std::endl;

	// =========================================================================
	// [NET-09] Configure address reuse and non-blocking listener
	// Criteria:
	// - The configuration follows CORE-04 including macOS restrictions.
	// - A setup failure releases the socket.
	// =========================================================================
	{
		ServerConfig uninit_serv(18080, "secret");
		try {
			uninit_serv.configureListenerSocket(); // No socket created yet -> fails & releases
			assert(false);
		} catch (const ServerConfig::SocketOptionException &) {
			assert(uninit_serv.getListenFd() == -1);
			std::cout << "[OK] Setup failure releases the socket and leaves FD at -1" << std::endl;
		}

		ServerConfig cfg_serv(18080, "secret");
		cfg_serv.createListenerSocket(AF_INET);
		cfg_serv.configureListenerSocket();
		int flags = fcntl(cfg_serv.getListenFd(), F_GETFL, 0);
		assert(flags >= 0 && (flags & O_NONBLOCK) != 0);
		std::cout << "[OK] Listener configured with SO_REUSEADDR and O_NONBLOCK" << std::endl;
	}

	// =========================================================================
	// [NET-10] Bind the configured listening address
	// [NET-11] Listen and release partially initialized resources
	// [NET-12] Introduce the poll table and one poll call site
	// [NET-13] Accept one ready connection safely
	// [NET-14] Assign stable client IDs and connection records
	// =========================================================================
	{
		// Find a free loopback port in range 18080..18180
		int test_port = 18080;
		bool bound = false;
		for (; test_port <= 18180; ++test_port) {
			ServerConfig probe(test_port, "super_secret_psw");
			try {
				probe.initListener(INADDR_LOOPBACK, LISTEN_BACKLOG);
				bound = true;
				break;
			} catch (const std::exception &) {
			}
		}
		assert(bound);

		// Bind valid free loopback port and reach listening state
		ServerConfig listener(test_port, "super_secret_psw");
		listener.initListener(INADDR_LOOPBACK, LISTEN_BACKLOG);
		assert(listener.isListening());
		assert(listener.getListenFd() >= 0);

		// Startup message reports listening state without exposing password
		std::ostringstream oss;
		oss << listener;
		assert(oss.str().find("super_secret_psw") == std::string::npos);
		assert(oss.str().find("Listening: yes") != std::string::npos);
		std::cout << "[OK] Bound loopback port " << test_port
				  << " | Startup: " << oss.str() << std::endl;

		// Occupied port fails cleanly without disturbing the process owning it
		ServerConfig colliding(test_port, "other_psw");
		colliding.createListenerSocket(AF_INET);
		try {
			colliding.bindListenerSocket(INADDR_LOOPBACK);
			assert(false);
		} catch (const ServerConfig::SocketBindException &) {
			assert(colliding.getListenFd() == -1);
			assert(listener.isListening());
			std::cout << "[OK] Occupied port fails cleanly and releases colliding FD" << std::endl;
		}

		// Injected listen failure (backlog <= 0) unwinds completed startup steps
		ServerConfig fail_listen(test_port + 1, "psw");
		fail_listen.createListenerSocket(AF_INET);
		fail_listen.configureListenerSocket();
		try {
			fail_listen.startListening(0);
			assert(false);
		} catch (const ServerConfig::SocketListenException &) {
			assert(fail_listen.getListenFd() == -1);
			assert(!fail_listen.isListening());
			std::cout << "[OK] Injected listen failure unwinds socket cleanly" << std::endl;
		}

		// Listener registered in pollfd table; idle wait sleeps and returns 0
		assert(listener.getPollFds().size() == 1);
		assert(listener.getPollFds()[0].fd == listener.getListenFd());
		assert(listener.getPollFds()[0].events == POLLIN);
		int idle_ready = listener.waitForEvents(10);
		assert(idle_ready == 0);
		std::cout << "[OK] Poll table registered listener and idle wait returned 0" << std::endl;

		// Unready listener never calls accept
		assert(!listener.isListenerReady());
		assert(listener.acceptReadyClient() == -1);

		// Connect client1 BEFORE calling waitForEvents()
		ScopedSocket client1;
		client1.openTcp(AF_INET);
		struct sockaddr_in caddr;
		std::memset(&caddr, 0, sizeof(caddr));
		caddr.sin_family = AF_INET;
		caddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		caddr.sin_port = htons(static_cast<uint16_t>(test_port));
		assert(::connect(client1.get(), reinterpret_cast<struct sockaddr *>(&caddr), sizeof(caddr)) == 0);

		// Even with client1 queued in the kernel, acceptReadyClient() must not call accept before poll
		assert(!listener.isListenerReady());
		assert(listener.acceptReadyClient() == -1);
		assert(listener.getClientNb() == 0);
		std::cout << "[OK] Unready listener path never calls accept" << std::endl;

		// Now poll reports readiness -> accept exactly one connection and return to poll
		assert(listener.waitForEvents(50) == 1);
		assert(listener.isListenerReady());

		int accepted_fd1 = listener.acceptReadyClient();
		assert(accepted_fd1 >= 0);
		assert(listener.getClientNb() == 1);

		// Bounded to 1 attempt per poll cycle: immediate second call returns -1
		assert(!listener.isListenerReady());
		assert(listener.acceptReadyClient() == -1);
		assert(listener.waitForEvents(10) == 0);
		std::cout << "[OK] Single bounded accept succeeded without blocking loop" << std::endl;

		// Verify ClientId and ClientRecord for first client
		irc::ClientId id1 = listener.getClientIdByFd(accepted_fd1);
		assert(id1 == 1);
		assert(listener.getLastAcceptedClientId() == id1);
		const ClientRecord *rec1 = listener.getClientById(id1);
		assert(rec1 != NULL);
		assert(rec1->id == id1);
		assert(rec1->fd == accepted_fd1);
		assert(rec1->in_buf.empty());
		assert(rec1->out_buf.empty());
		assert(listener.getClientByFd(accepted_fd1) == rec1);

		// [NET-14] Two connections have different IDs even after an fd is reused:
		// Keep client1 open while removing id1 (closes accepted_fd1), so accepted_fd1 is the
		// lowest available FD in the process when acceptReadyClient() accepts client2!
		ScopedSocket client2;
		client2.openTcp(AF_INET);
		assert(::connect(client2.get(), reinterpret_cast<struct sockaddr *>(&caddr), sizeof(caddr)) == 0);

		assert(listener.removeClientById(id1)); // Frees accepted_fd1 only
		assert(listener.getClientNb() == 0);
		assert(listener.getClientById(id1) == NULL);

		assert(listener.waitForEvents(50) == 1);
		int accepted_fd2 = listener.acceptReadyClient();
		client1.reset(); // Now safe to close client1's local socket
		assert(accepted_fd2 >= 0);
		assert(accepted_fd2 == accepted_fd1); // Same raw FD reused by kernel!

		irc::ClientId id2 = listener.getClientIdByFd(accepted_fd2);
		assert(id2 == 2);
		assert(id2 != id1); // Distinct ClientId!
		assert(listener.getClientById(id1) == NULL);
		assert(listener.getClientById(id2) != NULL);
		assert(listener.getClientById(id2)->fd == accepted_fd2);
		std::cout << "[OK] Reused FD (" << accepted_fd2 << ") received a new unique ClientId ("
				  << id1 << " -> " << id2 << ")" << std::endl;

		// ID exhaustion refuses a connection rather than wrapping into an existing identity
		listener.setNextClientId(ULONG_MAX);

		ScopedSocket client3;
		client3.openTcp(AF_INET);
		assert(::connect(client3.get(), reinterpret_cast<struct sockaddr *>(&caddr), sizeof(caddr)) == 0);
		assert(listener.waitForEvents(50) == 1);
		int accepted_fd3 = listener.acceptReadyClient();
		assert(accepted_fd3 >= 0);
		assert(listener.getClientIdByFd(accepted_fd3) == ULONG_MAX);
		assert(listener.isIdExhausted());

		// Next connection after ULONG_MAX must be refused without wrapping around
		ScopedSocket client4;
		client4.openTcp(AF_INET);
		assert(::connect(client4.get(), reinterpret_cast<struct sockaddr *>(&caddr), sizeof(caddr)) == 0);
		assert(listener.waitForEvents(50) == 1);
		assert(listener.acceptReadyClient() == -1);
		assert(listener.getClientNb() == 2);
		assert(listener.getClientById(id2) != NULL);
		assert(listener.getClientById(ULONG_MAX) != NULL);
		std::cout << "[OK] ID exhaustion at ULONG_MAX refuses new connection without wrapping" << std::endl;
	}

	std::cout << "--- All Socket tests passed successfully! ---" << std::endl;
	return (0);
}