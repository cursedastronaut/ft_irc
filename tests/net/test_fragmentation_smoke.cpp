#include "Test.hpp"
#include "contracts/Protocol.hpp"
#include "net/ScopedSocket.hpp"
#include "net/ServerConfig.hpp"

#include <arpa/inet.h>
#include <csignal>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <map>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
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

class RecordingEchoProtocol : public protocol
{
private:
	transport												*_transport;
	std::vector< std::pair<irc::ClientId, std::string> >	_recorded;

public:
	RecordingEchoProtocol() : _transport(0), _recorded() {}
	virtual ~RecordingEchoProtocol() {}

	void	setTransport(transport *tr) { _transport = tr; }

	virtual void	connected(irc::ClientId) {}

	virtual void	online(irc::ClientId id, const std::string &line)
	{
		_recorded.push_back(std::make_pair(id, line));
		if (_transport != 0)
			_transport->enqueueLine(id, "ACK " + line);
	}

	virtual void	disconnected(irc::ClientId, const std::string &) {}

	std::string	serializeLog() const
	{
		std::ostringstream	oss;
		for (std::size_t i = 0; i < _recorded.size(); ++i)
			oss << _recorded[i].first << '\t' << _recorded[i].second << '\n';
		return oss.str();
	}
};

bool	writeAllFd(int fd, const char *data, std::size_t len)
{
	std::size_t	written = 0;
	while (written < len)
	{
		const ssize_t	n = ::write(fd, data + written, len - written);
		if (n <= 0)
			return false;
		written += static_cast<std::size_t>(n);
	}
	return true;
}

bool	readAllPipeWithTimeout(int fd, std::string &out, int timeout_ms)
{
	int		remaining = timeout_ms;
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
		const ssize_t	n = ::read(fd, buf, sizeof(buf));
		if (n > 0)
			out.append(buf, static_cast<std::size_t>(n));
		else if (n == 0)
			return true;
		else
			return false;
	}
	return false;
}

int	runRecordingServerChild(int ready_write_fd, int log_write_fd)
{
	try
	{
		if (!irc::ServerConfig::installShutdownSignals())
			return 2;

		RecordingEchoProtocol	stub;
		irc::ServerConfig		serv(0, "net37_frag_secret", &stub);
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

		const std::string	log_data = stub.serializeLog();
		if (!log_data.empty() && !writeAllFd(log_write_fd, log_data.data(), log_data.size()))
		{
			::close(log_write_fd);
			return 5;
		}
		::close(log_write_fd);
		return 0;
	}
	catch (...)
	{
		if (ready_write_fd >= 0)
			::close(ready_write_fd);
		if (log_write_fd >= 0)
			::close(log_write_fd);
		return 6;
	}
}

class ScopedRecordingServer
{
private:
	pid_t		_pid;
	uint16_t	_port;
	ScopedFd	_log_read_fd;
	bool		_reaped;

	ScopedRecordingServer(const ScopedRecordingServer &);
	ScopedRecordingServer &operator=(const ScopedRecordingServer &);

	int	waitChild(int grace_ms)
	{
		if (_pid <= 0 || _reaped)
			return 0;
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
	ScopedRecordingServer() : _pid(-1), _port(0), _log_read_fd(-1), _reaped(false) {}

	~ScopedRecordingServer()
	{
		if (_pid > 0 && !_reaped)
		{
			::kill(_pid, SIGTERM);
			_log_read_fd.reset(-1);
			waitChild(1000);
		}
	}

	void	start()
	{
		int	ready_pipe[2];
		int	log_pipe[2];
		CHECK(::pipe(ready_pipe) == 0);
		ScopedFd	ready_r(ready_pipe[0]);
		ScopedFd	ready_w(ready_pipe[1]);
		CHECK(::pipe(log_pipe) == 0);
		ScopedFd	log_r(log_pipe[0]);
		ScopedFd	log_w(log_pipe[1]);

		_pid = ::fork();
		CHECK(_pid >= 0);
		if (_pid == 0)
		{
			ready_r.reset(-1);
			log_r.reset(-1);
			const int	rc = runRecordingServerChild(ready_w.get(), log_w.get());
			::_exit(rc);
		}
		ready_w.reset(-1);
		log_w.reset(-1);

		std::string	port_text;
		CHECK(readAllPipeWithTimeout(ready_r.get(), port_text, 2000));
		ready_r.reset(-1);

		std::istringstream	iss(port_text);
		unsigned int		parsed_port = 0;
		CHECK((iss >> parsed_port) && parsed_port > 0 && parsed_port <= 65535);
		_port = static_cast<uint16_t>(parsed_port);

		// Transfer ownership of log read end
		_log_read_fd.reset(log_r.get());
		// Prevent double-close by releasing raw fd manually
		// (swap via temporary dup-free pattern: keep log_r closed after reset)
		// Safer: read via _log_read_fd directly by duplicating or avoiding ScopedFd for log_r.
	}

	// Clean version without double-close risk:
	void	startClean()
	{
		int	ready_pipe[2];
		int	log_pipe[2];
		CHECK(::pipe(ready_pipe) == 0);
		ScopedFd	ready_r(ready_pipe[0]);
		ScopedFd	ready_w(ready_pipe[1]);
		CHECK(::pipe(log_pipe) == 0);
		_log_read_fd.reset(log_pipe[0]);
		ScopedFd	log_w(log_pipe[1]);

		_pid = ::fork();
		CHECK(_pid >= 0);
		if (_pid == 0)
		{
			ready_r.reset(-1);
			_log_read_fd.reset(-1);
			const int	rc = runRecordingServerChild(ready_w.get(), log_w.get());
			::_exit(rc);
		}
		ready_w.reset(-1);
		log_w.reset(-1);

		std::string	port_text;
		CHECK(readAllPipeWithTimeout(ready_r.get(), port_text, 2000));
		ready_r.reset(-1);

		std::istringstream	iss(port_text);
		unsigned int		parsed_port = 0;
		CHECK((iss >> parsed_port) && parsed_port > 0 && parsed_port <= 65535);
		_port = static_cast<uint16_t>(parsed_port);
	}

	uint16_t	port() const { return _port; }

	void	stopAndCollectLog(std::string &out_log)
	{
		::kill(_pid, SIGTERM);
		CHECK(readAllPipeWithTimeout(_log_read_fd.get(), out_log, 2000));
		_log_read_fd.reset(-1);
		const int	status = waitChild(2000);
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

bool	expectNoImmediateReply(int fd, int wait_ms)
{
	struct pollfd	pfd;
	pfd.fd = fd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	return (::poll(&pfd, 1, wait_ms) == 0);
}

std::map<irc::ClientId, std::vector<std::string> >	parseRecordedLog(
		const std::string &raw_log)
{
	std::map<irc::ClientId, std::vector<std::string> >	by_client;
	std::istringstream									iss(raw_log);
	std::string											entry;
	while (std::getline(iss, entry))
	{
		const std::string::size_type	tab = entry.find('\t');
		CHECK(tab != std::string::npos);
		std::istringstream	id_stream(entry.substr(0, tab));
		irc::ClientId		cid = 0;
		CHECK((id_stream >> cid) && cid != 0);
		by_client[cid].push_back(entry.substr(tab + 1));
	}
	return by_client;
}

std::vector<std::string>	buildCommandSequence()
{
	std::vector<std::string>	cmds;
	cmds.push_back("PASS net37_frag_secret");
	cmds.push_back("NICK alice");
	cmds.push_back("USER alice 0 * :Alice Example");
	cmds.push_back("JOIN #general");
	cmds.push_back("PRIVMSG #general :hello fragmented world");
	cmds.push_back("PING :coalesced_budget_check");
	cmds.push_back("NOTICE #general :partial tail");
	return cmds;
}

static void	fragmentationFixturesMatchUnsplitBaseline()
{
	ScopedRecordingServer	server;
	server.startClean();

	const std::vector<std::string>	commands = buildCommandSequence();

	// Client 1 (ID 1): Unsplit baseline (one complete CRLF line per send)
	std::vector<std::string>	baseline_replies;
	{
		irc::ScopedSocket	c1;
		std::string			buf;
		connectClient(c1, server.port());
		for (std::size_t i = 0; i < commands.size(); ++i)
		{
			sendAll(c1.get(), commands[i] + "\r\n");
			std::string	reply;
			CHECK(readWireLine(c1.get(), buf, reply, 2000));
			CHECK(reply == "ACK " + commands[i] + "\r\n");
			baseline_replies.push_back(reply);
		}
	}

	// Client 2 (ID 2): Unsplit bare-LF baseline for the subject's nc command
	std::string	nc_baseline_reply;
	{
		irc::ScopedSocket	c2;
		std::string			buf;
		connectClient(c2, server.port());
		sendAll(c2.get(), "command\n");
		CHECK(readWireLine(c2.get(), buf, nc_baseline_reply, 2000));
		CHECK(nc_baseline_reply == "ACK command\r\n");
	}

	// Client 3 (ID 3): Subject's nc-style fragmentation ("com" ^D "man" ^D "d\n")
	{
		irc::ScopedSocket	c3;
		std::string			buf;
		connectClient(c3, server.port());

		sendAll(c3.get(), "com");
		CHECK(expectNoImmediateReply(c3.get(), 25));

		sendAll(c3.get(), "man");
		CHECK(expectNoImmediateReply(c3.get(), 25));

		sendAll(c3.get(), "d\n");
		std::string	reply;
		CHECK(readWireLine(c3.get(), buf, reply, 2000));
		CHECK(reply == nc_baseline_reply);
		CHECK(expectNoImmediateReply(c3.get(), 20));
	}

	// Client 4 (ID 4): Split-byte input (1 byte per send + split CRLF check)
	std::vector<std::string>	split_replies;
	{
		irc::ScopedSocket	c4;
		std::string			buf;
		connectClient(c4, server.port());

		for (std::size_t i = 0; i < commands.size(); ++i)
		{
			const std::string	wire = commands[i] + "\r\n";
			for (std::size_t b = 0; b < wire.size(); ++b)
			{
				sendAll(c4.get(), wire.substr(b, 1));
				if (wire[b] == '\r')
					CHECK(expectNoImmediateReply(c4.get(), 15));
				else if (b + 1 < wire.size())
					::poll(0, 0, 1);
			}
			std::string	reply;
			CHECK(readWireLine(c4.get(), buf, reply, 2000));
			split_replies.push_back(reply);
		}
		CHECK(expectNoImmediateReply(c4.get(), 20));
	}
	CHECK(split_replies == baseline_replies);

	// Client 5 (ID 5): Combined / coalesced input (> DEFAULT_LINE_BUDGET in 1 send)
	std::vector<std::string>	combined_replies;
	{
		irc::ScopedSocket	c5;
		std::string			buf;
		connectClient(c5, server.port());

		CHECK(commands.size() > static_cast<std::size_t>(DEFAULT_LINE_BUDGET));
		std::string	combined_payload;
		for (std::size_t i = 0; i + 1 < commands.size(); ++i)
			combined_payload += commands[i] + "\r\n";
		const std::string	last_wire = commands.back() + "\r\n";
		const std::size_t	split_at = last_wire.size() / 2;
		combined_payload += last_wire.substr(0, split_at);

		// Single send with 6 complete lines + partial 7th line
		sendAll(c5.get(), combined_payload);

		// All 6 complete lines must drain across poll cycles without extra input
		for (std::size_t i = 0; i + 1 < commands.size(); ++i)
		{
			std::string	reply;
			CHECK(readWireLine(c5.get(), buf, reply, 2000));
			combined_replies.push_back(reply);
		}
		// Partial 7th line must not be dispatched yet
		CHECK(expectNoImmediateReply(c5.get(), 25));

		// Finish the 7th line
		sendAll(c5.get(), last_wire.substr(split_at));
		std::string	last_reply;
		CHECK(readWireLine(c5.get(), buf, last_reply, 2000));
		combined_replies.push_back(last_reply);
		CHECK(expectNoImmediateReply(c5.get(), 20));
	}
	CHECK(combined_replies == baseline_replies);

	// Client 6 (ID 6): Negative case - unterminated fragment + EOF produces no callback
	{
		irc::ScopedSocket	c6;
		connectClient(c6, server.port());
		sendAll(c6.get(), "UNTERMINATED_COMMAND_ONLY");
		CHECK(expectNoImmediateReply(c6.get(), 25));
	}

	// Client 7 (ID 7): Healthy client after unterminated EOF still works cleanly
	{
		irc::ScopedSocket	c7;
		std::string			buf;
		connectClient(c7, server.port());
		sendAll(c7.get(), "PING :after_unterminated_eof\r\n");
		std::string	reply;
		CHECK(readWireLine(c7.get(), buf, reply, 2000));
		CHECK(reply == "ACK PING :after_unterminated_eof\r\n");
	}

	// Stop server cleanly and verify recorded callbacks match unsplit baseline exactly
	const uint16_t	bound_port = server.port();
	std::string		raw_log;
	server.stopAndCollectLog(raw_log);
	CHECK(!tryConnectLoopback(bound_port));

	std::map<irc::ClientId, std::vector<std::string> >	callbacks =
			parseRecordedLog(raw_log);

	CHECK(callbacks[1] == commands);
	CHECK(callbacks[4] == callbacks[1]);
	CHECK(callbacks[5] == callbacks[1]);

	CHECK(callbacks[2].size() == 1);
	CHECK(callbacks[2][0] == "command");
	CHECK(callbacks[3] == callbacks[2]);

	// Client 6 (unterminated fragment + EOF) must have zero recorded callbacks
	CHECK(callbacks.find(6) == callbacks.end());

	CHECK(callbacks[7].size() == 1);
	CHECK(callbacks[7][0] == "PING :after_unterminated_eof");
}

} // namespace

int	main()
{
	irc::ServerConfig::ignoreSigpipe();
	return runTests("NET-37 repeatable fragmentation fixtures",
			fragmentationFixturesMatchUnsplitBaseline);
}
