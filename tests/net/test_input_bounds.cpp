#include "net/ServerConfig.hpp"
#include <arpa/inet.h>
#include <iostream>
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
	std::vector<std::pair<irc::ClientId, std::string> >	lines;
	std::vector<std::pair<irc::ClientId, std::string> >	disconnects;

	virtual void	connected(irc::ClientId id) {
		(void)id;
	}
	virtual void	online(irc::ClientId id, const std::string &line) {
		lines.push_back(std::make_pair(id, line));
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
}

int	main() {
	SpyProtocol			spy;
	irc::ServerConfig	serv(0, "testpw", &spy);
	serv.initListener(INADDR_LOOPBACK, 16);

	CHECK(serv.getMaxInputLineBytes() == 512);

	// 1. At-limit CRLF (510 payload + "\r\n" = 512 bytes) -> accepted
	irc::ClientId	c1 = 0;
	int				fd1 = connectLoopback(serv, c1);
	CHECK(fd1 >= 0 && c1 != 0);
	const std::string	exact_crlf_payload(510, 'A');
	const std::string	exact_crlf = exact_crlf_payload + "\r\n";
	CHECK(serv.handleRecvResult(c1, exact_crlf.data(),
			static_cast<ssize_t>(exact_crlf.size())) == 512);
	CHECK(!serv.isClientMarkedForCleanup(c1));
	CHECK(serv.dispatchClientLines(c1) == 1);
	CHECK(spy.lines.size() == 1 && spy.lines[0].second == exact_crlf_payload);

	// 2. At-limit bare LF (511 payload + "\n" = 512 bytes) -> accepted
	const std::string	exact_lf_payload(511, 'B');
	const std::string	exact_lf = exact_lf_payload + "\n";
	CHECK(serv.handleRecvResult(c1, exact_lf.data(),
			static_cast<ssize_t>(exact_lf.size())) == 512);
	CHECK(!serv.isClientMarkedForCleanup(c1));
	CHECK(serv.dispatchClientLines(c1) == 1);
	CHECK(spy.lines.size() == 2 && spy.lines[1].second == exact_lf_payload);

	// 3. At-limit fragmented CRLF (510 + "\r" then "\n") -> accepted
	const std::string	frag1 = std::string(510, 'C') + "\r";
	const std::string	frag2 = "\n";
	CHECK(serv.handleRecvResult(c1, frag1.data(),
			static_cast<ssize_t>(frag1.size())) == 511);
	CHECK(!serv.isClientMarkedForCleanup(c1));
	CHECK(serv.handleRecvResult(c1, frag2.data(), 1) == 1);
	CHECK(!serv.isClientMarkedForCleanup(c1));
	CHECK(serv.dispatchClientLines(c1) == 1);
	CHECK(spy.lines.size() == 3 && spy.lines[2].second == std::string(510, 'C'));

	// 4. Over-limit complete CRLF (511 + "\r\n" = 513 bytes) -> rejected
	irc::ClientId	c2 = 0;
	int				fd2 = connectLoopback(serv, c2);
	CHECK(fd2 >= 0 && c2 != 0);
	const std::string	over_crlf = std::string(511, 'D') + "\r\n";
	CHECK(serv.handleRecvResult(c2, over_crlf.data(),
			static_cast<ssize_t>(over_crlf.size())) == -1);
	CHECK(serv.isClientMarkedForCleanup(c2));
	CHECK(serv.getClientById(c2)->in_buf.empty());
	CHECK(serv.dispatchClientLines(c2) == 0);
	CHECK(serv.cleanupMarkedClients() == 1);
	CHECK(serv.getClientById(c2) == NULL);
	CHECK(spy.disconnects.size() == 1 && spy.disconnects[0].first == c2);

	// 5. Over-limit complete bare LF (512 + "\n" = 513 bytes) -> rejected
	irc::ClientId	c3 = 0;
	int				fd3 = connectLoopback(serv, c3);
	CHECK(fd3 >= 0 && c3 != 0);
	const std::string	over_lf = std::string(512, 'E') + "\n";
	CHECK(serv.handleRecvResult(c3, over_lf.data(),
			static_cast<ssize_t>(over_lf.size())) == -1);
	CHECK(serv.isClientMarkedForCleanup(c3));
	CHECK(serv.cleanupMarkedClients() == 1);

	// 6. Hostile never-terminated stream cannot grow buffer indefinitely
	irc::ClientId	c4 = 0;
	int				fd4 = connectLoopback(serv, c4);
	CHECK(fd4 >= 0 && c4 != 0);
	const std::string	chunk(100, 'X');
	for (int i = 0; i < 5; ++i) {
		CHECK(serv.handleRecvResult(c4, chunk.data(),
				static_cast<ssize_t>(chunk.size())) == 100);
		CHECK(!serv.isClientMarkedForCleanup(c4));
		CHECK(serv.getClientById(c4)->in_buf.size() < 512);
	}
	// 500 + 12 = 512 bytes without '\n' -> rejected immediately
	const std::string	tip(12, 'X');
	CHECK(serv.handleRecvResult(c4, tip.data(),
			static_cast<ssize_t>(tip.size())) == -1);
	CHECK(serv.isClientMarkedForCleanup(c4));
	CHECK(serv.getClientById(c4)->in_buf.empty());
	CHECK(serv.cleanupMarkedClients() == 1);

	// 7. Hostile pipelined short-line flood hits total input buffer cap
	irc::ClientId	c5 = 0;
	int				fd5 = connectLoopback(serv, c5);
	CHECK(fd5 >= 0 && c5 != 0);
	serv.setMaxInputBufferBytes(512);
	CHECK(serv.getMaxInputBufferBytes() == 512);
	const std::string	short_lines = "A\nB\nC\nD\nE\nF\nG\nH\n"; // 16 bytes
	for (int i = 0; i < 32; ++i) {
		CHECK(serv.handleRecvResult(c5, short_lines.data(),
				static_cast<ssize_t>(short_lines.size())) == 16);
	}
	CHECK(serv.handleRecvResult(c5, short_lines.data(),
			static_cast<ssize_t>(short_lines.size())) == -1);
	CHECK(serv.isClientMarkedForCleanup(c5));
	CHECK(serv.getClientById(c5)->in_buf.empty());
	CHECK(serv.cleanupMarkedClients() == 1);

	// Normal client c1 remained intact throughout
	CHECK(serv.getClientById(c1) != NULL);
	CHECK(!serv.isClientMarkedForCleanup(c1));

	::close(fd1);
	::close(fd2);
	::close(fd3);
	::close(fd4);
	::close(fd5);
	serv.shutdown();
	std::cout << "test_input_bounds: PASS" << std::endl;
	return (0);
}
