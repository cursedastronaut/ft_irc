#include "net/ServerConfig.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#define CHECK(cond) \
	do { \
		if (!(cond)) { \
			std::cerr << "CHECK FAILED at " << __FILE__ << ":" << __LINE__ \
					  << ": " #cond << std::endl; \
			return (1); \
		} \
	} while (0)

namespace
{

class BudgetSpyProtocol : public protocol
{
public:
	irc::ServerConfig							*transport_ref;
	std::map<irc::ClientId, std::vector<std::string> >	lines_by_client;
	std::vector<irc::ClientId>					disconnected_ids;

	explicit BudgetSpyProtocol(irc::ServerConfig *t = NULL) : transport_ref(t) {}

	virtual void	connected(irc::ClientId id) {
		lines_by_client[id];
	}

	virtual void	online(irc::ClientId id, const std::string &line) {
		lines_by_client[id].push_back(line);
		if (transport_ref == NULL)
			return ;
		if (line.find("USER ") == 0)
			transport_ref->enqueueLine(id, ":ircserv 001 alice :Welcome to ircserv");
		else if (line.find("PRIVMSG ") == 0)
			transport_ref->enqueueLine(id, ":alice!u@h " + line);
		else if (line.find("PING ") == 0)
			transport_ref->enqueueLine(id, ":ircserv PONG ircserv :" + line.substr(5));
	}

	virtual void	disconnected(irc::ClientId id, const std::string &reason) {
		(void)reason;
		disconnected_ids.push_back(id);
	}
};

int	findFreeLoopbackPort() {
	int	s = ::socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		return (-1);
	struct sockaddr_in	addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(0);
	if (::bind(s, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0) {
		::close(s);
		return (-1);
	}
	socklen_t	len = sizeof(addr);
	if (::getsockname(s, reinterpret_cast<struct sockaddr *>(&addr), &len) < 0) {
		::close(s);
		return (-1);
	}
	int	port = ntohs(addr.sin_port);
	::close(s);
	return (port);
}

int	connectLoopback(int port) {
	int	fd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		return (-1);
	struct sockaddr_in	addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(static_cast<uint16_t>(port));
	if (::connect(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0) {
		::close(fd);
		return (-1);
	}
	::fcntl(fd, F_SETFL, O_NONBLOCK);
	return (fd);
}

bool	sendAllNonBlocking(int fd, const std::string &data) {
	size_t	sent = 0;
	for (int attempts = 0; sent < data.size() && attempts < 200; ++attempts) {
		ssize_t	n = ::send(fd, data.data() + sent, data.size() - sent, 0);
		if (n > 0) {
			sent += static_cast<size_t>(n);
		} else {
			struct pollfd	pfd;
			pfd.fd = fd;
			pfd.events = POLLOUT;
			pfd.revents = 0;
			::poll(&pfd, 1, 10);
		}
	}
	return (sent == data.size());
}

void	drainSocketNonBlocking(int fd, std::string &out) {
	char	buf[512];
	for (int i = 0; i < 64; ++i) {
		struct pollfd	pfd;
		pfd.fd = fd;
		pfd.events = POLLIN;
		pfd.revents = 0;
		if (::poll(&pfd, 1, 0) <= 0 || (pfd.revents & POLLIN) == 0)
			break ;
		ssize_t	n = ::recv(fd, buf, sizeof(buf), 0);
		if (n <= 0)
			break ;
		out.append(buf, static_cast<size_t>(n));
	}
}

void	runOneLoopStep(irc::ServerConfig &serv, int default_timeout_ms) {
	int	timeout = serv.getEffectivePollTimeout(default_timeout_ms);
	int	ready = serv.waitForEvents(timeout);
	if (ready > 0) {
		if (serv.isListenerReady())
			(void)serv.acceptReadyClient();
		serv.readAllReadyClients();
		serv.writeAllReadyClients();
	}
	serv.dispatchAllReadyLines();
	serv.markErroredClientsForCleanup();
	serv.enforceFlushCloseBounds();
	serv.cleanupMarkedClients();
}

int	testHealthyPeerUnderFloodAndNoStranding() {
	int	port = findFreeLoopbackPort();
	CHECK(port > 0);

	BudgetSpyProtocol	proto;
	irc::ServerConfig	serv(port, "secret", &proto);
	proto.transport_ref = &serv;

	serv.setLineBudget(4);
	serv.setDispatchByteBudget(128);
	serv.setReadByteBudget(128);
	serv.setWriteByteBudget(128);
	serv.setInBufHighWater(512);
	serv.initListener(INADDR_LOOPBACK, LISTEN_BACKLOG);

	int	flood_fd = connectLoopback(port);
	CHECK(flood_fd >= 0);
	for (int i = 0; i < 20 && serv.getClientNb() < 1; ++i)
		runOneLoopStep(serv, 20);
	CHECK(serv.getClientNb() == 1);
	irc::ClientId	flood_id = serv.getLastAcceptedClientId();

	int	healthy_fd = connectLoopback(port);
	CHECK(healthy_fd >= 0);
	for (int i = 0; i < 20 && serv.getClientNb() < 2; ++i)
		runOneLoopStep(serv, 20);
	CHECK(serv.getClientNb() == 2);
	irc::ClientId	healthy_id = serv.getLastAcceptedClientId();

	const size_t	kFloodLines = 40;
	std::string		flood_payload;
	for (size_t i = 0; i < kFloodLines; ++i) {
		std::ostringstream	oss;
		oss << "PING flood_" << i << "_0123456789abcdef0123456789\r\n";
		flood_payload += oss.str();
	}
	std::string	healthy_payload =
		"PASS secret\r\n"
		"NICK alice\r\n"
		"USER alice 0 * :Alice\r\n"
		"PRIVMSG #room :hello\r\n";

	CHECK(sendAllNonBlocking(flood_fd, flood_payload));
	CHECK(sendAllNonBlocking(healthy_fd, healthy_payload));

	std::string	healthy_received;
	std::string	flood_received;
	bool		healthy_completed_while_flood_active = false;

	for (int iter = 0; iter < 200; ++iter) {
		size_t	flood_before = proto.lines_by_client[flood_id].size();
		runOneLoopStep(serv, 10);
		size_t	flood_after = proto.lines_by_client[flood_id].size();

		// Per-iteration line budget must never be exceeded.
		CHECK((flood_after - flood_before) <= serv.getLineBudget());

		drainSocketNonBlocking(healthy_fd, healthy_received);
		drainSocketNonBlocking(flood_fd, flood_received);

		if (!healthy_completed_while_flood_active
				&& proto.lines_by_client[healthy_id].size() == 4
				&& healthy_received.find("001 alice") != std::string::npos
				&& healthy_received.find("PRIVMSG #room :hello") != std::string::npos) {
			if (flood_after < kFloodLines)
				healthy_completed_while_flood_active = true;
		}
		if (flood_after == kFloodLines
				&& !serv.hasPendingCompleteLines()
				&& !serv.hasClientPendingOutput(flood_id)
				&& !serv.hasClientPendingOutput(healthy_id)) {
			drainSocketNonBlocking(flood_fd, flood_received);
			break ;
		}
	}

	CHECK(healthy_completed_while_flood_active);
	CHECK(proto.lines_by_client[healthy_id].size() == 4);
	CHECK(proto.lines_by_client[flood_id].size() == kFloodLines);
	CHECK(!serv.hasPendingCompleteLines());
	CHECK(!serv.hasClientPendingOutput(flood_id));
	CHECK(!serv.isClientWriteInterestEnabled(flood_id));
	CHECK(serv.isClientReadInterestEnabled(flood_id));

	// Verify last flood reply arrived intact (no stranded write leftover).
	CHECK(flood_received.find("PONG ircserv :flood_39_") != std::string::npos);

	::close(flood_fd);
	::close(healthy_fd);
	serv.shutdown("Test complete");
	return (0);
}

int	testOversizedLineExceedingByteBudgetStillProgresses() {
	int	port = findFreeLoopbackPort();
	CHECK(port > 0);

	BudgetSpyProtocol	proto;
	irc::ServerConfig	serv(port, "secret", &proto);
	proto.transport_ref = &serv;

	serv.setLineBudget(4);
	serv.setDispatchByteBudget(16);
	serv.initListener(INADDR_LOOPBACK, LISTEN_BACKLOG);

	int	peer_fd = connectLoopback(port);
	CHECK(peer_fd >= 0);
	for (int i = 0; i < 20 && serv.getClientNb() < 1; ++i)
		runOneLoopStep(serv, 20);
	CHECK(serv.getClientNb() == 1);
	irc::ClientId	cid = serv.getLastAcceptedClientId();

	std::string	two_long_lines =
		"PRIVMSG #a :1234567890123456789012345678\r\n"
		"PRIVMSG #b :abcdefghijabcdefghijabcdefgh\r\n";
	CHECK(sendAllNonBlocking(peer_fd, two_long_lines));

	// Wait until data is read, then verify dispatch happens 1 line at a time.
	for (int i = 0; i < 10 && !serv.hasClientCompleteLine(cid); ++i) {
		if (serv.waitForEvents(20) > 0)
			serv.readAllReadyClients();
	}
	CHECK(serv.hasClientCompleteLine(cid));
	CHECK(serv.dispatchClientLines(cid) == 1);
	CHECK(serv.hasPendingCompleteLines());
	CHECK(serv.getEffectivePollTimeout(100) == 0);
	CHECK(serv.dispatchClientLines(cid) == 1);
	CHECK(!serv.hasPendingCompleteLines());
	CHECK(proto.lines_by_client[cid].size() == 2);

	::close(peer_fd);
	serv.shutdown("Test complete");
	return (0);
}

int	testBudgetedWriteFlushBeforeClose() {
	int	port = findFreeLoopbackPort();
	CHECK(port > 0);

	BudgetSpyProtocol	proto;
	irc::ServerConfig	serv(port, "secret", &proto);
	proto.transport_ref = &serv;
	serv.setWriteByteBudget(32);
	serv.initListener(INADDR_LOOPBACK, LISTEN_BACKLOG);

	int	peer_fd = connectLoopback(port);
	CHECK(peer_fd >= 0);
	for (int i = 0; i < 20 && serv.getClientNb() < 1; ++i)
		runOneLoopStep(serv, 20);
	CHECK(serv.getClientNb() == 1);
	irc::ClientId	cid = serv.getLastAcceptedClientId();

	std::string	big_line(100, 'X');
	CHECK(serv.enqueueLine(cid, big_line) == Queued);
	serv.requestDisconnection(cid, "Closing after budgeted flush", true);
	CHECK(serv.isClientClosingAfterFlush(cid));

	// First write cycle must send at most 32 bytes and retain POLLOUT.
	CHECK(serv.waitForEvents(50) > 0);
	ssize_t	first_sent = serv.writeReadyClient(cid);
	CHECK(first_sent > 0 && first_sent <= 32);
	CHECK(serv.hasClientPendingOutput(cid));
	CHECK(serv.isClientWriteInterestEnabled(cid));

	std::string	received;
	for (int i = 0; i < 40 && serv.getClientNb() > 0; ++i) {
		runOneLoopStep(serv, 20);
		drainSocketNonBlocking(peer_fd, received);
	}
	drainSocketNonBlocking(peer_fd, received);

	CHECK(serv.getClientNb() == 0);
	CHECK(received == big_line + "\r\n");

	::close(peer_fd);
	serv.shutdown("Test complete");
	return (0);
}

}	//	namespace

int	main() {
	if (testHealthyPeerUnderFloodAndNoStranding() != 0)
		return (1);
	if (testOversizedLineExceedingByteBudgetStillProgresses() != 0)
		return (1);
	if (testBudgetedWriteFlushBeforeClose() != 0)
		return (1);
	std::cout << "test_work_budgets: ALL PASSED" << std::endl;
	return (0);
}
