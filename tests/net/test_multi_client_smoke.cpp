#include "Test.hpp"
#include "contracts/Protocol.hpp"
#include "net/ScopedSocket.hpp"
#include "net/ServerConfig.hpp"

#include <arpa/inet.h>
#include <csignal>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#ifdef MSG_NOSIGNAL
# define FIXTURE_SEND_FLAGS MSG_NOSIGNAL
#else
# define FIXTURE_SEND_FLAGS 0
#endif

namespace
{

class ScopedFd
{
private:
	int	_fd;

	ScopedFd(const ScopedFd &);
	ScopedFd &operator=(const ScopedFd &);

public:
	explicit ScopedFd(int fd = -1) : _fd(fd) {}
	~ScopedFd() { reset(-1); }

	int		get() const { return _fd; }
	bool	isValid() const { return _fd >= 0; }

	void	reset(int new_fd = -1)
	{
		if (_fd >= 0 && _fd != new_fd)
			::close(_fd);
		_fd = new_fd;
	}
};

class EchoStubProtocol : public protocol
{
private:
	transport	*_transport;

public:
	EchoStubProtocol() : _transport(0) {}
	virtual ~EchoStubProtocol() {}

	void	setTransport(transport *tr) { _transport = tr; }

	virtual void	connected(irc::ClientId) {}

	virtual void	online(irc::ClientId id, const std::string &line)
	{
		if (_transport != 0)
			_transport->enqueueLine(id, "ACK " + line);
	}

	virtual void	disconnected(irc::ClientId, const std::string &) {}
};

bool	writeAllFd(int fd, const char *data, std::size_t len)
{
	std::size_t	written = 0;
	while (written < len)
	{
		ssize_t	n = ::write(fd, data + written, len - written);
		if (n <= 0)
			return false;
		written += static_cast<std::size_t>(n);
	}
	return true;
}

bool	readAllPipeWithTimeout(int fd, std::string &out, int timeout_ms)
{
	int	remaining = timeout_ms;
	char	buf[256];
	while (remaining > 0)
	{
		struct pollfd	pfd;
		pfd.fd = fd;
		pfd.events = POLLIN;
		pfd.revents = 0;
		const int	step = (remaining > 20) ? 20 : remaining;
		const int	r = ::poll(&pfd, 1, step);
		if (r < 0)
			return false;
		if (r == 0)
		{
			remaining -= step;
			continue;
		}
		ssize_t	n = ::read(fd, buf, sizeof(buf));
		if (n > 0)
			out.append(buf, static_cast<std::size_t>(n));
		else if (n == 0)
			return true;
		else
			return false;
	}
	return false;
}

int	runStubServerChild(int ready_write_fd)
{
	try
	{
		if (!irc::ServerConfig::installShutdownSignals())
			return 2;

		EchoStubProtocol	stub;
		irc::ServerConfig	serv(0, "net36_smoke_secret", &stub);
		stub.setTransport(&serv);
		serv.initListener(INADDR_LOOPBACK, LISTEN_BACKLOG);

		struct sockaddr_in	addr;
		socklen_t			len = sizeof(addr);
		std::memset(&addr, 0, sizeof(addr));
		if (::getsockname(serv.getListenFd(),
				reinterpret_cast<struct sockaddr *>(&addr), &len) < 0)
		{
			serv.closeListener();
			return 3;
		}
		const uint16_t	port = ntohs(addr.sin_port);
		std::ostringstream	oss;
		oss << port << '\n';
		const std::string	port_line = oss.str();
		if (!writeAllFd(ready_write_fd, port_line.data(), port_line.size()))
		{
			serv.closeListener();
			return 4;
		}
		::close(ready_write_fd);
		ready_write_fd = -1;

		while (serv.isListening() && !irc::ServerConfig::isStopRequested())
		{
			const int	timeout = serv.getEffectivePollTimeout(POLL_TIMEOUT_MS);
			const int	ready = serv.waitForEvents(timeout);
			if (irc::ServerConfig::isStopRequested())
				break;
			if (ready > 0)
			{
				if (serv.isListenerReady())
				{
					const int	client_fd = serv.acceptReadyClient();
					(void)client_fd;
				}
				serv.readAllReadyClients();
				serv.writeAllReadyClients();
			}
			if (irc::ServerConfig::isStopRequested())
				break;
			serv.dispatchAllReadyLines();
			serv.markErroredClientsForCleanup();
			serv.enforceFlushCloseBounds();
			serv.cleanupMarkedClients();
		}
		serv.closeListener();
		return 0;
	}
	catch (...)
	{
		if (ready_write_fd >= 0)
			::close(ready_write_fd);
		return 5;
	}
}

class ScopedServerProcess
{
private:
	pid_t		_pid;
	uint16_t	_port;
	bool		_reaped;

	ScopedServerProcess(const ScopedServerProcess &);
	ScopedServerProcess &operator=(const ScopedServerProcess &);

	int	terminateAndWait(int grace_ms)
	{
		if (_pid <= 0 || _reaped)
			return 0;
		::kill(_pid, SIGTERM);
		int	status = 0;
		int	remaining = grace_ms;
		while (remaining > 0)
		{
			const pid_t	res = ::waitpid(_pid, &status, WNOHANG);
			if (res == _pid)
			{
				_reaped = true;
				return status;
			}
			if (res < 0)
			{
				_reaped = true;
				return -1;
			}
			const int	step = (remaining > 20) ? 20 : remaining;
			::poll(0, 0, step);
			remaining -= step;
		}
		::kill(_pid, SIGKILL);
		::waitpid(_pid, &status, 0);
		_reaped = true;
		return status;
	}

public:
	ScopedServerProcess() : _pid(-1), _port(0), _reaped(false) {}

	~ScopedServerProcess()
	{
		if (_pid > 0 && !_reaped)
			terminateAndWait(1000);
	}

	void	start()
	{
		int	ready_pipe[2];
		CHECK(::pipe(ready_pipe) == 0);
		ScopedFd	read_end(ready_pipe[0]);
		ScopedFd	write_end(ready_pipe[1]);

		_pid = ::fork();
		CHECK(_pid >= 0);
		if (_pid == 0)
		{
			read_end.reset(-1);
			const int	fd = write_end.get();
			const int	rc = runStubServerChild(fd);
			::_exit(rc);
		}
		write_end.reset(-1);

		std::string	port_text;
		CHECK(readAllPipeWithTimeout(read_end.get(), port_text, 2000));
		read_end.reset(-1);

		std::istringstream	iss(port_text);
		unsigned int		parsed_port = 0;
		CHECK((iss >> parsed_port) && parsed_port > 0 && parsed_port <= 65535);
		_port = static_cast<uint16_t>(parsed_port);
	}

	uint16_t	port() const { return _port; }

	void	stopCleanly()
	{
		const int	status = terminateAndWait(2000);
		CHECK(WIFEXITED(status));
		CHECK(WEXITSTATUS(status) == 0);
	}
};

void	connectClient(irc::ScopedSocket &sock, uint16_t port)
{
	sock.openTcp(AF_INET);
	int	one = 1;
	::setsockopt(sock.get(), IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
	struct sockaddr_in	addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(port);
	CHECK(::connect(sock.get(),
			reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) == 0);
}

bool	tryConnectLoopback(uint16_t port)
{
	irc::ScopedSocket	probe;
	probe.openTcp(AF_INET);
	struct sockaddr_in	addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(port);
	return (::connect(probe.get(),
			reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) == 0);
}

void	sendAll(int fd, const std::string &payload, int timeout_ms = 2000)
{
	std::size_t	offset = 0;
	int			remaining = timeout_ms;
	while (offset < payload.size())
	{
		struct pollfd	pfd;
		pfd.fd = fd;
		pfd.events = POLLOUT;
		pfd.revents = 0;
		const int	step = (remaining > 20) ? 20 : remaining;
		CHECK(step > 0);
		const int	r = ::poll(&pfd, 1, step);
		CHECK(r >= 0);
		if (r == 0)
		{
			remaining -= step;
			continue;
		}
		const ssize_t	n = ::send(fd, payload.data() + offset,
				payload.size() - offset, FIXTURE_SEND_FLAGS);
		CHECK(n > 0);
		offset += static_cast<std::size_t>(n);
	}
}

bool	readWireLine(int fd, std::string &buffer, std::string &out_line,
			int timeout_ms = 2000)
{
	int		remaining = timeout_ms;
	char	chunk[256];
	while (buffer.find('\n') == std::string::npos)
	{
		if (remaining <= 0)
			return false;
		struct pollfd	pfd;
		pfd.fd = fd;
		pfd.events = POLLIN;
		pfd.revents = 0;
		const int	step = (remaining > 20) ? 20 : remaining;
		const int	r = ::poll(&pfd, 1, step);
		if (r < 0)
			return false;
		if (r == 0)
		{
			remaining -= step;
			continue;
		}
		const ssize_t	n = ::recv(fd, chunk, sizeof(chunk), 0);
		if (n <= 0)
			return false;
		buffer.append(chunk, static_cast<std::size_t>(n));
	}
	const std::string::size_type	pos = buffer.find('\n');
	out_line.assign(buffer, 0, pos + 1);
	buffer.erase(0, pos + 1);
	return true;
}

bool	expectPeerEof(int fd, int timeout_ms = 2000)
{
	int	remaining = timeout_ms;
	while (remaining > 0)
	{
		struct pollfd	pfd;
		pfd.fd = fd;
		pfd.events = POLLIN;
		pfd.revents = 0;
		const int	step = (remaining > 20) ? 20 : remaining;
		const int	r = ::poll(&pfd, 1, step);
		if (r < 0)
			return false;
		if (r == 0)
		{
			remaining -= step;
			continue;
		}
		char			byte = 0;
		const ssize_t	n = ::recv(fd, &byte, 1, 0);
		return (n == 0);
	}
	return false;
}

void	exchangeStubRequestReply(int fd, std::string &recv_buf,
			const std::string &request)
{
	sendAll(fd, request + "\r\n");
	std::string	reply;
	CHECK(readWireLine(fd, recv_buf, reply, 2000));
	CHECK(reply == "ACK " + request + "\r\n");
}

static void	multiClientRepeatedConnectEofAndContinuedService()
{
	ScopedServerProcess	server;
	server.start();

	irc::ScopedSocket	survivor;
	std::string			survivor_buf;
	connectClient(survivor, server.port());
	exchangeStubRequestReply(survivor.get(), survivor_buf, "PING :survivor_init");

	// 4 rounds x 5 simultaneous clients = 20 connect/EOF cycles (> MAX_CLIENT)
	const std::size_t	rounds = 4;
	const std::size_t	batch_size = 5;
	for (std::size_t round = 0; round < rounds; ++round)
	{
		irc::ScopedSocket	churn[batch_size];
		std::string			churn_bufs[batch_size];

		for (std::size_t i = 0; i < batch_size; ++i)
			connectClient(churn[i], server.port());

		// Client 0 and 2 exchange a valid stub request/reply concurrently
		std::ostringstream	req0;
		req0 << "PING :round_" << round << "_c0";
		std::ostringstream	req2;
		req2 << "PING :round_" << round << "_c2";
		sendAll(churn[0].get(), req0.str() + "\r\n");
		sendAll(churn[2].get(), req2.str() + "\r\n");

		// Client 1 sends a partial unterminated fragment before EOF
		sendAll(churn[1].get(), "PARTIAL_BEFORE_EOF");

		// Client 3 disconnects immediately without sending any byte
		churn[3].reset(-1);

		// Verify replies for active churn clients while others close
		std::string	reply0;
		std::string	reply2;
		CHECK(readWireLine(churn[0].get(), churn_bufs[0], reply0, 2000));
		CHECK(reply0 == "ACK " + req0.str() + "\r\n");
		CHECK(readWireLine(churn[2].get(), churn_bufs[2], reply2, 2000));
		CHECK(reply2 == "ACK " + req2.str() + "\r\n");

		// Close remaining churn clients in non-linear order (middle, first, last)
		::shutdown(churn[2].get(), SHUT_WR);
		churn[2].reset(-1);
		churn[0].reset(-1);
		churn[1].reset(-1);
		churn[4].reset(-1);

		// Survivor still exchanges stub request/reply after every churn round
		std::ostringstream	surv_req;
		surv_req << "PING :survivor_after_round_" << round;
		exchangeStubRequestReply(survivor.get(), survivor_buf, surv_req.str());
	}

	// Negative / capacity case: saturate MAX_CLIENT and verify overflow is refused
	const std::size_t	max_clients = static_cast<std::size_t>(MAX_CLIENT);
	CHECK(max_clients >= 2);
	const std::size_t	filler_count = max_clients - 1;
	std::vector<int>	fillers;
	fillers.reserve(filler_count);
	try
	{
		for (std::size_t i = 0; i < filler_count; ++i)
		{
			irc::ScopedSocket	tmp;
			connectClient(tmp, server.port());
			std::string	filler_buf;
			std::ostringstream	ready_req;
			ready_req << "PING :fill_" << i;
			exchangeStubRequestReply(tmp.get(), filler_buf, ready_req.str());
			fillers.push_back(tmp.release());
		}

		// One extra connection beyond MAX_CLIENT must be closed by server
		irc::ScopedSocket	overflow_peer;
		connectClient(overflow_peer, server.port());
		CHECK(expectPeerEof(overflow_peer.get(), 2000));
		overflow_peer.reset(-1);

		// Remaining survivor client still works even at full capacity
		exchangeStubRequestReply(survivor.get(), survivor_buf, "PING :at_capacity");
	}
	catch (...)
	{
		for (std::size_t i = 0; i < fillers.size(); ++i)
			::close(fillers[i]);
		throw;
	}
	for (std::size_t i = 0; i < fillers.size(); ++i)
		::close(fillers[i]);
	fillers.clear();

	// After fillers disconnect, a fresh client can connect and survivor still works
	exchangeStubRequestReply(survivor.get(), survivor_buf, "PING :survivor_final");
	irc::ScopedSocket	fresh_client;
	std::string			fresh_buf;
	connectClient(fresh_client, server.port());
	exchangeStubRequestReply(fresh_client.get(), fresh_buf, "PING :fresh_after_drain");

	fresh_client.reset(-1);
	survivor.reset(-1);

	const uint16_t	bound_port = server.port();
	server.stopCleanly();
	CHECK(!tryConnectLoopback(bound_port));
}

} // namespace

int	main()
{
	irc::ServerConfig::ignoreSigpipe();
	return runTests("NET-36 multi-client connect and EOF smoke",
			multiClientRepeatedConnectEofAndContinuedService);
}
