#include "net/ServerConfig.hpp"
#include <arpa/inet.h>
#include <csignal>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
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

class CleanupSpyProtocol : public protocol
{
public:
	std::vector<irc::ClientId>	disconnected_ids;
	std::vector<std::string>	reasons;

	virtual void	connected(irc::ClientId id) {
		(void)id;
	}
	virtual void	online(irc::ClientId id, const std::string &line) {
		(void)id;
		(void)line;
	}
	virtual void	disconnected(irc::ClientId id, const std::string &reason) {
		disconnected_ids.push_back(id);
		reasons.push_back(reason);
	}
};

class CleanupSpyChannelService : public irc::ChannelService
{
public:
	std::vector<irc::ClientId>	removed_ids;

	virtual void	handle(irc::ClientId actorId, const irc::ParsedCommand &command) {
		(void)actorId;
		(void)command;
	}
	virtual void	removeClient(irc::ClientId actorId) {
		removed_ids.push_back(actorId);
	}
	virtual std::vector<irc::ClientId>	peersOf(irc::ClientId actorId) const {
		(void)actorId;
		return (std::vector<irc::ClientId>());
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
	return (fd);
}

int	testControlledStopInterruptionNeitherCorruptsNorSpins() {
	CHECK(irc::ServerConfig::installShutdownSignals());
	int	port = findFreeLoopbackPort();
	CHECK(port > 0);

	CleanupSpyProtocol			proto;
	CleanupSpyChannelService	chans;
	irc::ServerConfig			serv(port, "secret", &proto, &chans);
	serv.initListener(INADDR_LOOPBACK, LISTEN_BACKLOG);

	int	peer_fd = connectLoopback(port);
	CHECK(peer_fd >= 0);
	for (int i = 0; i < 20 && serv.getClientNb() < 1; ++i) {
		if (serv.waitForEvents(20) > 0 && serv.isListenerReady())
			(void)serv.acceptReadyClient();
	}
	CHECK(serv.getClientNb() == 1);
	irc::ClientId	cid = serv.getLastAcceptedClientId();

	// Trigger real SIGINT handler, then verify interrupted poll return (-1)
	// preserves live state without throwing or spinning.
	CHECK(::raise(SIGINT) == 0);
	CHECK(irc::ServerConfig::isStopRequested());

	int	ready = serv.injectPollResult(-1);
	CHECK(ready == 0);
	CHECK(serv.getLastPollStatus() == irc::ServerConfig::PollStopRequested);
	CHECK(serv.isListening());
	CHECK(serv.getClientNb() == 1);
	CHECK(serv.getClientById(cid) != NULL);
	CHECK(proto.disconnected_ids.empty());

	// Subsequent waitForEvents exits immediately with stop status (no blocking).
	int	ready2 = serv.waitForEvents(200);
	CHECK(ready2 == 0);
	CHECK(serv.getLastPollStatus() == irc::ServerConfig::PollStopRequested);

	// Orderly shutdown invoked by loop exit cleans up everything once.
	serv.closeListener();
	CHECK(!serv.isListening());
	CHECK(serv.getClientNb() == 0);
	CHECK(proto.disconnected_ids.size() == 1 && proto.disconnected_ids[0] == cid);
	CHECK(chans.removed_ids.size() == 1 && chans.removed_ids[0] == cid);

	::close(peer_fd);
	irc::ServerConfig::resetStopFlag();
	return (0);
}

int	testUnrecoverablePollFailurePerformsOrderlyCleanup() {
	irc::ServerConfig::resetStopFlag();
	int	port = findFreeLoopbackPort();
	CHECK(port > 0);

	CleanupSpyProtocol			proto;
	CleanupSpyChannelService	chans;
	irc::ServerConfig			serv(port, "secret", &proto, &chans);
	serv.initListener(INADDR_LOOPBACK, LISTEN_BACKLOG);

	int	peer_fd = connectLoopback(port);
	CHECK(peer_fd >= 0);
	for (int i = 0; i < 20 && serv.getClientNb() < 1; ++i) {
		if (serv.waitForEvents(20) > 0 && serv.isListenerReady())
			(void)serv.acceptReadyClient();
	}
	CHECK(serv.getClientNb() == 1);
	irc::ClientId	cid = serv.getLastAcceptedClientId();

	bool	threw = false;
	try {
		(void)serv.injectPollResult(-1);
	} catch (const irc::ServerConfig::PollException &) {
		threw = true;
	}

	CHECK(threw);
	CHECK(serv.getLastPollStatus() == irc::ServerConfig::PollError);
	CHECK(!serv.isListening());
	CHECK(serv.getListenFd() == -1);
	CHECK(serv.getClientNb() == 0);
	CHECK(serv.getPollFds().empty());
	CHECK(serv.getClientFds().empty());
	CHECK(proto.disconnected_ids.size() == 1 && proto.disconnected_ids[0] == cid);
	CHECK(chans.removed_ids.size() == 1 && chans.removed_ids[0] == cid);

	::close(peer_fd);
	return (0);
}

int	testUnrecoverableListenerPollnvalAvoidsBusyLoop() {
	irc::ServerConfig::resetStopFlag();
	int	port = findFreeLoopbackPort();
	CHECK(port > 0);

	CleanupSpyProtocol			proto;
	CleanupSpyChannelService	chans;
	irc::ServerConfig			serv(port, "secret", &proto, &chans);
	serv.initListener(INADDR_LOOPBACK, LISTEN_BACKLOG);

	int	peer_fd = connectLoopback(port);
	CHECK(peer_fd >= 0);
	for (int i = 0; i < 20 && serv.getClientNb() < 1; ++i) {
		if (serv.waitForEvents(20) > 0 && serv.isListenerReady())
			(void)serv.acceptReadyClient();
	}
	CHECK(serv.getClientNb() == 1);
	irc::ClientId	cid = serv.getLastAcceptedClientId();

	// Force a real kernel POLLNVAL on the listener entry in _poll_fds[0].
	::close(serv.getListenFd());

	bool	threw = false;
	try {
		(void)serv.waitForEvents(50);
	} catch (const irc::ServerConfig::PollException &) {
		threw = true;
	}

	CHECK(threw);
	CHECK(serv.getLastPollStatus() == irc::ServerConfig::PollError);
	CHECK(!serv.isListening());
	CHECK(serv.getClientNb() == 0);
	CHECK(serv.getPollFds().empty());
	CHECK(proto.disconnected_ids.size() == 1 && proto.disconnected_ids[0] == cid);
	CHECK(chans.removed_ids.size() == 1 && chans.removed_ids[0] == cid);

	::close(peer_fd);
	return (0);
}

}	//	namespace

int	main() {
	if (testControlledStopInterruptionNeitherCorruptsNorSpins() != 0)
		return (1);
	if (testUnrecoverablePollFailurePerformsOrderlyCleanup() != 0)
		return (1);
	if (testUnrecoverableListenerPollnvalAvoidsBusyLoop() != 0)
		return (1);
	std::cout << "test_poll_error_policy: ALL PASSED" << std::endl;
	return (0);
}
