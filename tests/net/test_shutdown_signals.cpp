#include "net/ServerConfig.hpp"
#include <csignal>
#include <ctime>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

#define CHECK(cond) do { \
	if (!(cond)) { \
		std::cerr << "FAIL [" << __FILE__ << ":" << __LINE__ << "]: " #cond << std::endl; \
		return (1); \
	} \
} while (0)

namespace {

int	testSignalBreaksEventLoop(int signum) {
	irc::ServerConfig	serv(0, "test_pass");
	serv.initListener(INADDR_LOOPBACK, 8);
	CHECK(irc::ServerConfig::installShutdownSignals());
	CHECK(!irc::ServerConfig::isStopRequested());

	pid_t	parent = ::getpid();
	pid_t	child = ::fork();
	CHECK(child >= 0);
	if (child == 0) {
		::usleep(100000);
		::kill(parent, signum);
		::_exit(0);
	}

	std::time_t	before = std::time(NULL);
	int			iterations = 0;
	while (serv.isListening() && !irc::ServerConfig::isStopRequested()) {
		int	ready = serv.waitForEvents(5000);
		(void)ready;
		++iterations;
		if (irc::ServerConfig::isStopRequested())
			break ;
	}
	std::time_t	elapsed = std::time(NULL) - before;

	int	status = 0;
	::waitpid(child, &status, 0);

	CHECK(irc::ServerConfig::isStopRequested());
	CHECK(elapsed < 3);
	CHECK(iterations == 1);

	serv.closeListener();
	irc::ServerConfig::resetStopFlag();
	CHECK(!irc::ServerConfig::isStopRequested());
	return (0);
}

int	testPreSetStopFlagSkipsBlockingPoll() {
	irc::ServerConfig	serv(0, "test_pass");
	serv.initListener(INADDR_LOOPBACK, 8);
	CHECK(irc::ServerConfig::installShutdownSignals());

	::raise(SIGTERM);
	CHECK(irc::ServerConfig::isStopRequested());

	std::time_t	before = std::time(NULL);
	int			ready = serv.waitForEvents(5000);
	std::time_t	elapsed = std::time(NULL) - before;

	CHECK(ready == 0);
	CHECK(elapsed < 2);

	serv.closeListener();
	irc::ServerConfig::resetStopFlag();
	return (0);
}

} // namespace

int	main() {
	if (testSignalBreaksEventLoop(SIGINT) != 0)
		return (1);
	if (testSignalBreaksEventLoop(SIGTERM) != 0)
		return (1);
	if (testPreSetStopFlagSkipsBlockingPoll() != 0)
		return (1);
	std::cout << "NET-29 shutdown signal tests OK" << std::endl;
	return (0);
}
