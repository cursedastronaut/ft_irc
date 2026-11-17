#undef NDEBUG
#include <iostream>
#include <cassert>
#include <cstring>
#include <vector>
#include <string>
#include <utility>
#include <sys/socket.h>
#include "net/ServerConfig.hpp"
#include "contracts/Protocol.hpp"

namespace {

class LineCaptureStub : public protocol {
private:
	std::vector< std::pair<irc::ClientId, std::string> >	_events;
	std::map<irc::ClientId, std::vector<std::string> >		_lines_by_client;

public:
	LineCaptureStub() : _events(), _lines_by_client() {}
	virtual ~LineCaptureStub() {}

	virtual void connected(irc::ClientId id) {
		(void)id;
	}

	virtual void online(irc::ClientId id, const std::string &line) {
		_events.push_back(std::make_pair(id, line));
		_lines_by_client[id].push_back(line);
	}

	virtual void disconnected(irc::ClientId id, const std::string &reason) {
		(void)id;
		(void)reason;
	}

	void clear() {
		_events.clear();
		_lines_by_client.clear();
	}

	size_t totalLines() const {
		return (_events.size());
	}

	const std::vector<std::string> &linesFor(irc::ClientId id) const {
		static const std::vector<std::string> empty;
		std::map<irc::ClientId, std::vector<std::string> >::const_iterator it = _lines_by_client.find(id);
		if (it == _lines_by_client.end())
			return (empty);
		return (it->second);
	}
};

int findFreeLoopbackPort(int start_port, int end_port) {
	for (int port = start_port; port <= end_port; ++port) {
		irc::ServerConfig probe(port, "probe_secret");
		try {
			probe.initListener(INADDR_LOOPBACK, LISTEN_BACKLOG);
			return (port);
		} catch (const std::exception &) {
		}
	}
	return (-1);
}

void connectLoopbackClient(irc::ScopedSocket &client_sock, int port) {
	client_sock.openTcp(AF_INET);
	struct sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(static_cast<uint16_t>(port));
	int rc = ::connect(client_sock.get(), reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
	assert(rc == 0);
}

void sendBytes(int fd, const char *data, size_t len) {
	ssize_t sent = ::send(fd, data, len, 0);
	assert(sent == static_cast<ssize_t>(len));
}

void sendStr(int fd, const char *s) {
	sendBytes(fd, s, std::strlen(s));
}

} // namespace

int main(void) {
	int port = findFreeLoopbackPort(18500, 18700);
	assert(port > 0);

	LineCaptureStub stub;
	irc::ServerConfig server(port, "net18_21_secret", &stub);
	server.initListener(INADDR_LOOPBACK, LISTEN_BACKLOG);

	// =========================================================================
	// [NET-18] Handle EOF and negative recv results
	// =========================================================================
	std::cout << "--- [NET-18] EOF and Negative Recv Result Tests ---" << std::endl;
	{
		irc::ScopedSocket c1_eof;
		irc::ScopedSocket c2_neg;
		irc::ScopedSocket c3_healthy;

		connectLoopbackClient(c1_eof, port);
		assert(server.waitForEvents(50) == 1);
		int fd1 = server.acceptReadyClient();
		irc::ClientId id1 = server.getClientIdByFd(fd1);

		connectLoopbackClient(c2_neg, port);
		assert(server.waitForEvents(50) == 1);
		int fd2 = server.acceptReadyClient();
		irc::ClientId id2 = server.getClientIdByFd(fd2);

		connectLoopbackClient(c3_healthy, port);
		assert(server.waitForEvents(50) == 1);
		int fd3 = server.acceptReadyClient();
		irc::ClientId id3 = server.getClientIdByFd(fd3);
		assert(server.getClientNb() == 3);

		// 1. Peer closes connection -> recv() == 0 marks ONLY id1 for cleanup
		c1_eof.reset();
		assert(server.waitForEvents(50) == 1);
		assert(server.isClientReadable(id1));
		assert(server.readReadyClient(id1) == 0);
		assert(server.isClientMarkedForCleanup(id1));
		assert(!server.isClientMarkedForCleanup(id2));
		assert(!server.isClientMarkedForCleanup(id3));

		// 2. Injected negative recv result marks ONLY id2 for cleanup
		assert(server.injectRecvResult(id2, -1) == -1);
		assert(server.isClientMarkedForCleanup(id2));
		assert(!server.isClientMarkedForCleanup(id3));

		// 3. Cleanup removes only id1 and id2; id3 remains active and readable
		assert(server.cleanupMarkedClients() == 2);
		assert(server.getClientById(id1) == NULL);
		assert(server.getClientById(id2) == NULL);
		assert(server.getClientById(id3) != NULL);
		assert(server.getClientNb() == 1);

		sendStr(c3_healthy.get(), "PING healthy\r\n");
		assert(server.waitForEvents(50) == 1);
		assert(server.readReadyClient(id3) > 0);
		assert(server.dispatchClientLines(id3) == 1);
		assert(stub.linesFor(id3).size() == 1);
		assert(stub.linesFor(id3)[0] == "PING healthy");

		assert(server.removeClientById(id3));
		stub.clear();
		std::cout << "[OK] EOF and injected negative results mark and remove only affected clients" << std::endl;
	}

	// =========================================================================
	// [NET-19] Extract complete CRLF-terminated lines
	// =========================================================================
	std::cout << "--- [NET-19] Complete CRLF Line Extraction Tests ---" << std::endl;
	{
		irc::ScopedSocket client;
		connectLoopbackClient(client, port);
		assert(server.waitForEvents(50) == 1);
		int fd = server.acceptReadyClient();
		irc::ClientId id = server.getClientIdByFd(fd);

		// Two complete lines + one unfinished line
		sendStr(client.get(), "NICK neo\r\nUSER neo 0 * :Anderson\r\nJOIN #zion");
		assert(server.waitForEvents(50) == 1);
		assert(server.readReadyClient(id) > 0);

		assert(server.dispatchClientLines(id) == 2);
		assert(stub.linesFor(id).size() == 2);
		assert(stub.linesFor(id)[0] == "NICK neo");
		assert(stub.linesFor(id)[1] == "USER neo 0 * :Anderson");

		// Unfinished line remains buffered and undispatched
		assert(server.getClientById(id)->in_buf == "JOIN #zion");
		assert(server.dispatchClientLines(id) == 0);
		assert(stub.linesFor(id).size() == 2);

		assert(server.removeClientById(id));
		stub.clear();
		std::cout << "[OK] Two complete lines dispatched in order; unfinished line kept in buffer" << std::endl;
	}

	// =========================================================================
	// [NET-20] Preserve split CRLF and fragmented commands
	// =========================================================================
	std::cout << "--- [NET-20] Split CRLF and Fragmented Command Tests ---" << std::endl;
	{
		irc::ScopedSocket client;
		connectLoopbackClient(client, port);
		assert(server.waitForEvents(50) == 1);
		int fd = server.acceptReadyClient();
		irc::ClientId id = server.getClientIdByFd(fd);

		// 1. Subject's fragmented command example: "com" + "man" + "d\n"
		const char *fragments[3] = { "com", "man", "d\n" };
		for (size_t i = 0; i < 3; ++i) {
			sendStr(client.get(), fragments[i]);
			assert(server.waitForEvents(50) == 1);
			assert(server.readReadyClient(id) > 0);
			size_t dispatched = server.dispatchClientLines(id);
			if (i < 2)
				assert(dispatched == 0);
			else
				assert(dispatched == 1);
		}
		assert(stub.linesFor(id).size() == 1);
		assert(stub.linesFor(id)[0] == "command");
		assert(server.getClientById(id)->in_buf.empty());

		// 2. Final CR delivered separately from LF ("PRIVMSG #a :hi\r" then "\n")
		sendStr(client.get(), "PRIVMSG #a :hi\r");
		assert(server.waitForEvents(50) == 1);
		assert(server.readReadyClient(id) > 0);
		assert(server.dispatchClientLines(id) == 0);
		assert(stub.linesFor(id).size() == 1);

		sendStr(client.get(), "\n");
		assert(server.waitForEvents(50) == 1);
		assert(server.readReadyClient(id) > 0);
		assert(server.dispatchClientLines(id) == 1);
		assert(stub.linesFor(id).size() == 2);
		assert(stub.linesFor(id)[1] == "PRIVMSG #a :hi");
		assert(server.getClientById(id)->in_buf.empty());

		assert(server.removeClientById(id));
		stub.clear();
		std::cout << "[OK] Fragmented command and split CR/LF reassembled once without extra empty line" << std::endl;
	}

	// =========================================================================
	// [NET-21] Dispatch coalesced input without dropping lines
	// =========================================================================
	std::cout << "--- [NET-21] Coalesced Input and Per-Cycle Budget Tests ---" << std::endl;
	{
		// 1. Combined registration buffer yields exact same line sequence as separate sends
		irc::ScopedSocket c_separate;
		connectLoopbackClient(c_separate, port);
		assert(server.waitForEvents(50) == 1);
		irc::ClientId id_sep = server.getClientIdByFd(server.acceptReadyClient());

		const char *reg_lines[3] = {
			"PASS secret\r\n",
			"NICK trinity\r\n",
			"USER trinity 0 * :Trinity\r\n"
		};
		for (size_t i = 0; i < 3; ++i) {
			sendStr(c_separate.get(), reg_lines[i]);
			assert(server.waitForEvents(50) == 1);
			assert(server.readReadyClient(id_sep) > 0);
			assert(server.dispatchClientLines(id_sep) == 1);
		}

		irc::ScopedSocket c_coalesced;
		connectLoopbackClient(c_coalesced, port);
		assert(server.waitForEvents(50) == 1);
		irc::ClientId id_coal = server.getClientIdByFd(server.acceptReadyClient());

		// Set budget to 2 lines per cycle: 5 coalesced lines + 1 unfinished tail
		server.setLineBudget(2);
		sendStr(c_coalesced.get(),
			"PASS secret\r\n"
			"NICK trinity\r\n"
			"USER trinity 0 * :Trinity\r\n"
			"JOIN #matrix\r\n"
			"PRIVMSG #matrix :hello\r\n"
			"PARTIAL_TAIL");

		// Single recv reads the entire coalesced payload
		assert(server.waitForEvents(50) == 1);
		assert(server.readReadyClient(id_coal) > 0);

		// Cycle 1 dispatches 2 lines (budget = 2) and retains excess work
		assert(server.dispatchAllReadyLines() == 2);
		assert(stub.linesFor(id_coal).size() == 2);
		assert(server.hasPendingCompleteLines());
		assert(server.getEffectivePollTimeout(100) == 0);

		// Cycle 2 resumes WITHOUT another network byte (poll timeout = 0)
		assert(server.waitForEvents(server.getEffectivePollTimeout(100)) == 0);
		assert(server.dispatchAllReadyLines() == 2);
		assert(stub.linesFor(id_coal).size() == 4);
		assert(server.hasPendingCompleteLines());
		assert(server.getEffectivePollTimeout(100) == 0);

		// Cycle 3 dispatches the 5th complete line; only "PARTIAL_TAIL" remains
		assert(server.waitForEvents(server.getEffectivePollTimeout(100)) == 0);
		assert(server.dispatchAllReadyLines() == 1);
		assert(stub.linesFor(id_coal).size() == 5);

		// First 3 registration lines match separate sends identically
		for (size_t i = 0; i < 3; ++i) {
			assert(stub.linesFor(id_coal)[i] == stub.linesFor(id_sep)[i]);
		}
		assert(stub.linesFor(id_coal)[3] == "JOIN #matrix");
		assert(stub.linesFor(id_coal)[4] == "PRIVMSG #matrix :hello");

		// No complete line left -> effective poll timeout returns to normal (no busy spin)
		assert(!server.hasPendingCompleteLines());
		assert(server.getEffectivePollTimeout(100) == 100);
		assert(server.getClientById(id_coal)->in_buf == "PARTIAL_TAIL");

		std::cout << "[OK] Coalesced lines dispatched in order across budgets without busy spinning" << std::endl;
	}

	std::cout << "--- All [NET-18..21] tests passed successfully! ---" << std::endl;
	return (0);
}
