#include "net/ServerConfig.hpp"
#include <arpa/inet.h>
#include <iostream>
#include <poll.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>
#include <vector>

#define CHECK(expr) do { \
	if (!(expr)) { \
		std::cerr << "FAIL (" << __FILE__ << ":" << __LINE__ << "): " #expr << std::endl; \
		return (1); \
	} \
} while (0)

namespace
{
class SpyProtocol : public protocol
{
public:
	std::vector<std::pair<irc::ClientId, std::string> >	disconnects;

	virtual void	connected(irc::ClientId id) { (void)id; }
	virtual void	online(irc::ClientId id, const std::string &line) {
		(void)id;
		(void)line;
	}
	virtual void	disconnected(irc::ClientId id, const std::string &reason) {
		disconnects.push_back(std::make_pair(id, reason));
	}
};

int	connectLoopback(irc::ServerConfig &serv, irc::ClientId &out_id) {
	struct sockaddr_in	bound;
	socklen_t			len = sizeof(bound);
	if (::getsockname(serv.getListenFd(),
			reinterpret_cast<struct sockaddr *>(&bound), &len) < 0)
		return (-1);

	int	cfd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (cfd < 0)
		return (-1);
	if (::connect(cfd, reinterpret_cast<struct sockaddr *>(&bound), len) < 0) {
		::close(cfd);
		return (-1);
	}
	if (serv.waitForEvents(500) <= 0 || !serv.isListenerReady()) {
		::close(cfd);
		return (-1);
	}
	if (serv.acceptReadyClient() < 0) {
		::close(cfd);
		return (-1);
	}
	out_id = serv.getLastAcceptedClientId();
	return (cfd);
}

bool	readAvailable(int fd, std::string &out) {
	struct pollfd	pfd;
	pfd.fd = fd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	if (::poll(&pfd, 1, 500) <= 0 || (pfd.revents & POLLIN) == 0)
		return (false);
	char	buf[256];
	ssize_t	n = ::recv(fd, buf, sizeof(buf), 0);
	if (n <= 0)
		return (false);
	out.append(buf, static_cast<size_t>(n));
	return (true);
}
}

int	main() {
	SpyProtocol			spy;
	irc::ServerConfig	serv(0, "testpw", &spy);
	serv.initListener(INADDR_LOOPBACK, 16);
	serv.setMaxOutputBufferBytes(40);

	irc::ClientId	normal_id = 0;
	irc::ClientId	slow_id = 0;
	int				normal_fd = connectLoopback(serv, normal_id);
	int				slow_fd = connectLoopback(serv, slow_id);
	CHECK(normal_fd >= 0 && slow_fd >= 0);

	// 1. Invalid line with embedded '\r'/'\n' still rejects WITHOUT teardown
	CHECK(serv.enqueueLine(normal_id, "BAD\r\nLINE") == Rejected);
	CHECK(!serv.isClientMarkedForCleanup(normal_id));

	// 2. Slow reader hits bounded rejection path without silent dropping
	// Each message is 18 bytes + "\r\n" = 20 bytes; 2 messages = 40 bytes (at-limit)
	CHECK(serv.enqueueLine(slow_id, "MSG_SLOW_000000001") == Queued);
	CHECK(serv.enqueueLine(slow_id, "MSG_SLOW_000000002") == Queued);
	CHECK(serv.getClientPendingOutputSize(slow_id) == 40);
	CHECK(!serv.isClientMarkedForCleanup(slow_id));

	// 3rd message would exceed 40 bytes -> rejected & teardown scheduled
	CHECK(serv.enqueueLine(slow_id, "MSG_SLOW_000000003") == Rejected);
	CHECK(serv.isClientMarkedForCleanup(slow_id));
	CHECK(serv.getClientPendingOutputSize(slow_id) == 0);
	CHECK(!serv.isClientWriteInterestEnabled(slow_id));
	// Subsequent enqueues stay rejected
	CHECK(serv.enqueueLine(slow_id, "AFTER") == Rejected);
	CHECK(serv.cleanupMarkedClients() == 1);
	CHECK(serv.getClientById(slow_id) == NULL);
	CHECK(spy.disconnects.size() == 1 && spy.disconnects[0].first == slow_id);

	// 3. Normal reader receives complete, ordered messages across multiple flushes
	// Total sent = 6 * 12 = 72 bytes (> 40-byte budget) while draining normally
	std::string	expected;
	std::string	received;
	for (int i = 1; i <= 6; ++i) {
		std::ostringstream	oss;
		oss << "NORMAL_00" << i;
		const std::string	msg = oss.str();
		expected += msg + "\r\n";

		CHECK(serv.enqueueLine(normal_id, msg) == Queued);
		CHECK(!serv.isClientMarkedForCleanup(normal_id));
		CHECK(serv.waitForEvents(500) > 0);
		CHECK(serv.writeAllReadyClients() >= 1);
		CHECK(readAvailable(normal_fd, received));
	}
	CHECK(received == expected);
	CHECK(serv.getClientPendingOutputSize(normal_id) == 0);
	CHECK(!serv.isClientMarkedForCleanup(normal_id));

	::close(normal_fd);
	::close(slow_fd);
	serv.shutdown();
	std::cout << "test_output_bounds: PASS" << std::endl;
	return (0);
}
