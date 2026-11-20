#include "Test.hpp"
#include "net/ServerConfig.hpp"

#include <climits>
#include <cstddef>
#include <cstring>
#include <fcntl.h>
#include <sstream>
#include <vector>

namespace
{

int getBoundPort(int listen_fd)
{
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	std::memset(&addr, 0, sizeof(addr));
	CHECK(::getsockname(listen_fd,
						reinterpret_cast<struct sockaddr *>(&addr), &len) == 0);
	return static_cast<int>(ntohs(addr.sin_port));
}

void connectLoopback(irc::ScopedSocket &client_sock, int port)
{
	client_sock.openTcp(AF_INET);
	struct sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(static_cast<uint16_t>(port));
	CHECK(::connect(client_sock.get(),
					reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) == 0);
}

static void scopedSocketEnforcesOwnershipAndRejectsInvalidInputs()
{
	bool bad_fd_rejected = false;
	try
	{
		irc::ScopedSocket bad_sock(-1);
	}
	catch (const irc::ScopedSocket::SocketCreationException &)
	{
		bad_fd_rejected = true;
	}
	CHECK(bad_fd_rejected);

	bool bad_family_rejected = false;
	try
	{
		irc::ScopedSocket bad_family;
		bad_family.openTcp(AF_UNIX);
	}
	catch (const irc::ScopedSocket::InvalidFamilyException &)
	{
		bad_family_rejected = true;
	}
	CHECK(bad_family_rejected);

	irc::ScopedSocket sock_v4;
	sock_v4.openTcp(AF_INET);
	CHECK(sock_v4.isValid());
	const int raw_fd = sock_v4.release();
	CHECK(!sock_v4.isValid());
	CHECK(sock_v4.get() == -1);

	irc::ScopedSocket owner2(raw_fd);
	irc::ScopedSocket owner3;
	owner2.swap(owner3);
	CHECK(!owner2.isValid());
	CHECK(owner3.get() == raw_fd);
	owner3.reset(-1);
	CHECK(!owner3.isValid());
}

static void listenerConfigurationAppliesNonBlockingAndUnwindsOnError()
{
	irc::ServerConfig uninit_serv(0, "fixture-secret");
	bool option_failed = false;
	try
	{
		uninit_serv.configureListenerSocket();
	}
	catch (const irc::ServerConfig::SocketOptionException &)
	{
		option_failed = true;
	}
	CHECK(option_failed);
	CHECK(uninit_serv.getListenFd() == -1);

	irc::ServerConfig cfg_serv(0, "fixture-secret");
	cfg_serv.createListenerSocket(AF_INET);
	cfg_serv.configureListenerSocket();
	const int flags = ::fcntl(cfg_serv.getListenFd(), F_GETFL, 0);
	CHECK(flags >= 0 && (flags & O_NONBLOCK) != 0);
}

static void listenerBindListenHandlesCollisionAndUnwindsFailures()
{
	irc::ServerConfig listener(0, "super_secret_psw");
	listener.initListener(INADDR_LOOPBACK, LISTEN_BACKLOG);
	CHECK(listener.isListening());
	CHECK(listener.getListenFd() >= 0);

	std::ostringstream oss;
	oss << listener;
	CHECK(oss.str().find("super_secret_psw") == std::string::npos);
	CHECK(oss.str().find("Listening: yes") != std::string::npos);

	const int bound_port = getBoundPort(listener.getListenFd());
	irc::ServerConfig colliding(bound_port, "other_psw");
	colliding.createListenerSocket(AF_INET);
	bool bind_failed = false;
	try
	{
		colliding.bindListenerSocket(INADDR_LOOPBACK);
	}
	catch (const irc::ServerConfig::SocketBindException &)
	{
		bind_failed = true;
	}
	CHECK(bind_failed);
	CHECK(colliding.getListenFd() == -1);
	CHECK(listener.isListening());

	irc::ServerConfig fail_listen(0, "psw");
	fail_listen.createListenerSocket(AF_INET);
	fail_listen.configureListenerSocket();
	bool listen_failed = false;
	try
	{
		fail_listen.startListening(0);
	}
	catch (const irc::ServerConfig::SocketListenException &)
	{
		listen_failed = true;
	}
	CHECK(listen_failed);
	CHECK(fail_listen.getListenFd() == -1);
	CHECK(!fail_listen.isListening());
}

static void pollAcceptAssignsUniqueIdsHandlesExhaustionAndKeepsNonBlocking()
{
	irc::ServerConfig listener(0, "super_secret_psw");
	listener.initListener(INADDR_LOOPBACK, LISTEN_BACKLOG);
	const int test_port = getBoundPort(listener.getListenFd());

	CHECK(listener.getPollFds().size() == 1);
	CHECK(listener.getPollFds()[0].fd == listener.getListenFd());
	CHECK(listener.getPollFds()[0].events == POLLIN);
	CHECK(listener.waitForEvents(10) == 0);

	// Unready listener must never call accept, even with a queued peer.
	CHECK(!listener.isListenerReady());
	CHECK(listener.acceptReadyClient() == -1);

	irc::ScopedSocket client1;
	connectLoopback(client1, test_port);
	CHECK(!listener.isListenerReady());
	CHECK(listener.acceptReadyClient() == -1);
	CHECK(listener.getClientNb() == 0);

	// Single bounded accept per poll cycle.
	CHECK(listener.waitForEvents(50) == 1);
	CHECK(listener.isListenerReady());
	const int accepted_fd1 = listener.acceptReadyClient();
	CHECK(accepted_fd1 >= 0);
	CHECK(listener.getClientNb() == 1);
	CHECK(!listener.isListenerReady());
	CHECK(listener.acceptReadyClient() == -1);
	CHECK(listener.waitForEvents(10) == 0);

	const irc::ClientId id1 = listener.getClientIdByFd(accepted_fd1);
	CHECK(id1 == 1);
	CHECK(listener.getLastAcceptedClientId() == id1);
	const irc::netClientRecord *rec1 = listener.getClientById(id1);
	CHECK(rec1 != 0);
	CHECK(rec1->id == id1);
	CHECK(rec1->fd == accepted_fd1);
	CHECK(rec1->in_buf.empty());
	CHECK(rec1->out_buf.empty());
	CHECK(listener.getClientByFd(accepted_fd1) == rec1);

	const int accepted_flags = ::fcntl(accepted_fd1, F_GETFL, 0);
	CHECK(accepted_flags >= 0 && (accepted_flags & O_NONBLOCK) != 0);

	// Reused raw FD receives a new unique ClientId.
	irc::ScopedSocket client2;
	connectLoopback(client2, test_port);
	CHECK(listener.removeClientById(id1));
	CHECK(listener.getClientNb() == 0);
	CHECK(listener.getClientById(id1) == 0);

	CHECK(listener.waitForEvents(50) == 1);
	const int accepted_fd2 = listener.acceptReadyClient();
	client1.reset();
	CHECK(accepted_fd2 >= 0);
	CHECK(accepted_fd2 == accepted_fd1);

	const irc::ClientId id2 = listener.getClientIdByFd(accepted_fd2);
	CHECK(id2 == 2);
	CHECK(id2 != id1);
	CHECK(listener.getClientById(id1) == 0);
	CHECK(listener.getClientById(id2) != 0);
	CHECK(listener.getClientById(id2)->fd == accepted_fd2);

	const std::vector<int> &client_fds = listener.getClientFds();
	CHECK(!client_fds.empty());
	for (std::size_t i = 0; i < client_fds.size(); ++i)
	{
		const int flags = ::fcntl(client_fds[i], F_GETFL, 0);
		CHECK(flags >= 0 && (flags & O_NONBLOCK) != 0);
	}

	// ID exhaustion at ULONG_MAX refuses new connections without wrapping.
	listener.setNextClientId(ULONG_MAX);
	irc::ScopedSocket client3;
	connectLoopback(client3, test_port);
	CHECK(listener.waitForEvents(50) == 1);
	const int accepted_fd3 = listener.acceptReadyClient();
	CHECK(accepted_fd3 >= 0);
	CHECK(listener.getClientIdByFd(accepted_fd3) == ULONG_MAX);
	CHECK(listener.isIdExhausted());

	irc::ScopedSocket client4;
	connectLoopback(client4, test_port);
	CHECK(listener.waitForEvents(50) == 1);
	CHECK(listener.acceptReadyClient() == -1);
	CHECK(listener.getClientNb() == 2);
	CHECK(listener.getClientById(id2) != 0);
	CHECK(listener.getClientById(ULONG_MAX) != 0);

	// Non-blocking flag survives poll cycles.
	CHECK(listener.waitForEvents(10) == 0);
	for (std::size_t i = 0; i < listener.getClientFds().size(); ++i)
	{
		const int flags = ::fcntl(listener.getClientFds()[i], F_GETFL, 0);
		CHECK(flags >= 0 && (flags & O_NONBLOCK) != 0);
	}
}

static void socketTests()
{
	scopedSocketEnforcesOwnershipAndRejectsInvalidInputs();
	listenerConfigurationAppliesNonBlockingAndUnwindsOnError();
	listenerBindListenHandlesCollisionAndUnwindsFailures();
	pollAcceptAssignsUniqueIdsHandlesExhaustionAndKeepsNonBlocking();
}

} // namespace

int main()
{
	return runTests("NET-08..15 sockets, poll, accept and client IDs", socketTests);
}
