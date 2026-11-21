#include "Test.hpp"
#include "contracts/Protocol.hpp"
#include "net/ServerConfig.hpp"

#include <cstddef>
#include <cstring>
#include <map>
#include <string>
#include <sys/socket.h>
#include <utility>
#include <vector>

namespace
{

class LineCaptureStub : public protocol
{
private:
	std::vector< std::pair<irc::ClientId, std::string> >	_events;
	std::map<irc::ClientId, std::vector<std::string> >		_lines_by_client;

public:
	LineCaptureStub() : _events(), _lines_by_client() {}
	virtual ~LineCaptureStub() {}

	virtual void connected(irc::ClientId) {}

	virtual void online(irc::ClientId id, const std::string &line)
	{
		_events.push_back(std::make_pair(id, line));
		_lines_by_client[id].push_back(line);
	}

	virtual void disconnected(irc::ClientId, const std::string &) {}

	const std::vector<std::string> &linesFor(irc::ClientId id) const
	{
		static const std::vector<std::string> empty;
		std::map<irc::ClientId, std::vector<std::string> >::const_iterator it =
			_lines_by_client.find(id);
		if (it == _lines_by_client.end())
			return empty;
		return it->second;
	}
};

int getBoundPort(int listen_fd)
{
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	std::memset(&addr, 0, sizeof(addr));
	CHECK(::getsockname(listen_fd,
						reinterpret_cast<struct sockaddr *>(&addr), &len) == 0);
	return static_cast<int>(ntohs(addr.sin_port));
}

irc::ClientId acceptLoopbackClient(irc::ServerConfig &server,
								   irc::ScopedSocket &client_sock)
{
	const int port = getBoundPort(server.getListenFd());
	client_sock.openTcp(AF_INET);
	struct sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(static_cast<uint16_t>(port));
	CHECK(::connect(client_sock.get(),
					reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) == 0);
	CHECK(server.waitForEvents(50) == 1);
	const int fd = server.acceptReadyClient();
	CHECK(fd >= 0);
	const irc::ClientId id = server.getClientIdByFd(fd);
	CHECK(id != 0);
	return id;
}

void sendStr(int fd, const char *s)
{
	const std::size_t len = std::strlen(s);
	const ssize_t sent = ::send(fd, s, len, 0);
	CHECK(sent == static_cast<ssize_t>(len));
}

static void eofAndNegativeRecvMarkOnlyAffectedClients()
{
	LineCaptureStub stub;
	irc::ServerConfig server(0, "net18_secret", &stub);
	server.initListener(INADDR_LOOPBACK, LISTEN_BACKLOG);

	irc::ScopedSocket c1_eof;
	irc::ScopedSocket c2_neg;
	irc::ScopedSocket c3_healthy;

	const irc::ClientId id1 = acceptLoopbackClient(server, c1_eof);
	const irc::ClientId id2 = acceptLoopbackClient(server, c2_neg);
	const irc::ClientId id3 = acceptLoopbackClient(server, c3_healthy);
	CHECK(server.getClientNb() == 3);

	c1_eof.reset();
	CHECK(server.waitForEvents(50) == 1);
	CHECK(server.isClientReadable(id1));
	CHECK(server.readReadyClient(id1) == 0);
	CHECK(server.isClientMarkedForCleanup(id1));
	CHECK(!server.isClientMarkedForCleanup(id2));
	CHECK(!server.isClientMarkedForCleanup(id3));

	CHECK(server.injectRecvResult(id2, -1) == -1);
	CHECK(server.isClientMarkedForCleanup(id2));
	CHECK(!server.isClientMarkedForCleanup(id3));

	CHECK(server.cleanupMarkedClients() == 2);
	CHECK(server.getClientById(id1) == 0);
	CHECK(server.getClientById(id2) == 0);
	CHECK(server.getClientById(id3) != 0);
	CHECK(server.getClientNb() == 1);

	sendStr(c3_healthy.get(), "PING healthy\r\n");
	CHECK(server.waitForEvents(50) == 1);
	CHECK(server.readReadyClient(id3) > 0);
	CHECK(server.dispatchClientLines(id3) == 1);
	CHECK(stub.linesFor(id3).size() == 1);
	CHECK(stub.linesFor(id3)[0] == "PING healthy");
}

static void completeCrlfLinesExtractInOrderAndKeepRemainder()
{
	LineCaptureStub stub;
	irc::ServerConfig server(0, "net19_secret", &stub);
	server.initListener(INADDR_LOOPBACK, LISTEN_BACKLOG);

	irc::ScopedSocket client;
	const irc::ClientId id = acceptLoopbackClient(server, client);

	sendStr(client.get(), "NICK neo\r\nUSER neo 0 * :Anderson\r\nJOIN #zion");
	CHECK(server.waitForEvents(50) == 1);
	CHECK(server.readReadyClient(id) > 0);

	CHECK(server.dispatchClientLines(id) == 2);
	CHECK(stub.linesFor(id).size() == 2);
	CHECK(stub.linesFor(id)[0] == "NICK neo");
	CHECK(stub.linesFor(id)[1] == "USER neo 0 * :Anderson");

	CHECK(server.getClientById(id)->in_buf == "JOIN #zion");
	CHECK(server.dispatchClientLines(id) == 0);
	CHECK(stub.linesFor(id).size() == 2);
}

static void fragmentedCommandsAndSplitCrlfReassembleOnce()
{
	LineCaptureStub stub;
	irc::ServerConfig server(0, "net20_secret", &stub);
	server.initListener(INADDR_LOOPBACK, LISTEN_BACKLOG);

	irc::ScopedSocket client;
	const irc::ClientId id = acceptLoopbackClient(server, client);

	const char *fragments[3] = { "com", "man", "d\n" };
	for (std::size_t i = 0; i < 3; ++i)
	{
		sendStr(client.get(), fragments[i]);
		CHECK(server.waitForEvents(50) == 1);
		CHECK(server.readReadyClient(id) > 0);
		const std::size_t dispatched = server.dispatchClientLines(id);
		CHECK(dispatched == (i < 2 ? 0U : 1U));
	}
	CHECK(stub.linesFor(id).size() == 1);
	CHECK(stub.linesFor(id)[0] == "command");
	CHECK(server.getClientById(id)->in_buf.empty());

	sendStr(client.get(), "PRIVMSG #a :hi\r");
	CHECK(server.waitForEvents(50) == 1);
	CHECK(server.readReadyClient(id) > 0);
	CHECK(server.dispatchClientLines(id) == 0);
	CHECK(stub.linesFor(id).size() == 1);

	sendStr(client.get(), "\n");
	CHECK(server.waitForEvents(50) == 1);
	CHECK(server.readReadyClient(id) > 0);
	CHECK(server.dispatchClientLines(id) == 1);
	CHECK(stub.linesFor(id).size() == 2);
	CHECK(stub.linesFor(id)[1] == "PRIVMSG #a :hi");
	CHECK(server.getClientById(id)->in_buf.empty());
}

static void coalescedInputRespectsLineBudgetWithoutBusySpin()
{
	LineCaptureStub stub;
	irc::ServerConfig server(0, "net21_secret", &stub);
	server.initListener(INADDR_LOOPBACK, LISTEN_BACKLOG);

	irc::ScopedSocket c_separate;
	const irc::ClientId id_sep = acceptLoopbackClient(server, c_separate);

	const char *reg_lines[3] = {
		"PASS secret\r\n",
		"NICK trinity\r\n",
		"USER trinity 0 * :Trinity\r\n"
	};
	for (std::size_t i = 0; i < 3; ++i)
	{
		sendStr(c_separate.get(), reg_lines[i]);
		CHECK(server.waitForEvents(50) == 1);
		CHECK(server.readReadyClient(id_sep) > 0);
		CHECK(server.dispatchClientLines(id_sep) == 1);
	}

	irc::ScopedSocket c_coalesced;
	const irc::ClientId id_coal = acceptLoopbackClient(server, c_coalesced);

	server.setLineBudget(2);
	sendStr(c_coalesced.get(),
			"PASS secret\r\n"
			"NICK trinity\r\n"
			"USER trinity 0 * :Trinity\r\n"
			"JOIN #matrix\r\n"
			"PRIVMSG #matrix :hello\r\n"
			"PARTIAL_TAIL");

	CHECK(server.waitForEvents(50) == 1);
	CHECK(server.readReadyClient(id_coal) > 0);

	CHECK(server.dispatchAllReadyLines() == 2);
	CHECK(stub.linesFor(id_coal).size() == 2);
	CHECK(server.hasPendingCompleteLines());
	CHECK(server.getEffectivePollTimeout(100) == 0);

	CHECK(server.waitForEvents(server.getEffectivePollTimeout(100)) == 0);
	CHECK(server.dispatchAllReadyLines() == 2);
	CHECK(stub.linesFor(id_coal).size() == 4);
	CHECK(server.hasPendingCompleteLines());
	CHECK(server.getEffectivePollTimeout(100) == 0);

	CHECK(server.waitForEvents(server.getEffectivePollTimeout(100)) == 0);
	CHECK(server.dispatchAllReadyLines() == 1);
	CHECK(stub.linesFor(id_coal).size() == 5);

	for (std::size_t i = 0; i < 3; ++i)
		CHECK(stub.linesFor(id_coal)[i] == stub.linesFor(id_sep)[i]);
	CHECK(stub.linesFor(id_coal)[3] == "JOIN #matrix");
	CHECK(stub.linesFor(id_coal)[4] == "PRIVMSG #matrix :hello");

	CHECK(!server.hasPendingCompleteLines());
	CHECK(server.getEffectivePollTimeout(100) == 100);
	CHECK(server.getClientById(id_coal)->in_buf == "PARTIAL_TAIL");
}

static void readLineTests()
{
	eofAndNegativeRecvMarkOnlyAffectedClients();
	completeCrlfLinesExtractInOrderAndKeepRemainder();
	fragmentedCommandsAndSplitCrlfReassembleOnce();
	coalescedInputRespectsLineBudgetWithoutBusySpin();
}

} // namespace

int main()
{
	return runTests("NET-18..21 recv, framing and line dispatch", readLineTests);
}
