#include "net/ServerConfig.hpp"
#include "net/ScopedShutdownGuard.hpp"

#ifdef MSG_NOSIGNAL
# define IRC_SEND_FLAGS MSG_NOSIGNAL
#else
# define IRC_SEND_FLAGS 0
#endif	//	MSG_NOSOGNAL

namespace
{
	volatile sig_atomic_t	g_stop_requested = 0;
}

extern "C" {
	static void	irc_handle_shutdown_signal(int signum) {
		(void)signum;
		g_stop_requested = 1;
	}
}

namespace irc 
{
//Private
void	ServerConfig::setClientWriteInterest_(int fd, bool enable) {
	if (fd < 0)
		return ;
	for(size_t i = 1; i <_poll_fds.size(); ++i) {
		if (_poll_fds[i].fd == fd) {
			if (enable)
				_poll_fds[i].events |= POLLOUT;
			else
				_poll_fds[i].events &= ~POLLOUT;
			break;
		}
	}
}

void	ServerConfig::stopAdmission_() {
	_is_listening = false;
	_listen_sock.reset();
	if (!_poll_fds.empty()) {
		_poll_fds[0].fd = -1;
		_poll_fds[0].events = 0;
		_poll_fds[0].revents = 0;
	}
}

//Public

//	Constructor
ServerConfig::ServerConfig(int irc_port, std::string psw,
		protocol *proto, ChannelService *channel_service)
	: _port(irc_port), _psw(psw), _client_nb(0), _listen_sock(),
		_is_listening(false), _poll_fds(), _client_fds(),
		_next_client_id(1), _id_exhausted(false), _last_accpted_id(0),
		_clients_by_id(), _fd_to_client_id(), _protocol(proto),
		_channel_service(channel_service), _line_budget(DEFAULT_LINE_BUDGET),
		_flush_timeout_sec(DEFAULT_FLUSH_TIMEOUT_SEC), _shutting_down(false) {
	ignoreSigpipe();
}

ServerConfig::ServerConfig(const ServerConfig &src)
	: _port(src._port), _psw(src._psw), _client_nb(src._client_nb),
		_listen_sock(), _is_listening(false), _poll_fds(), _client_fds(),
		_next_client_id(src._next_client_id), _id_exhausted(src._id_exhausted),
		_last_accpted_id(src._last_accpted_id), _clients_by_id(src._clients_by_id),
		_fd_to_client_id(src._fd_to_client_id), _protocol(src._protocol),
		_channel_service(src._channel_service), _line_budget(src._line_budget),
		_flush_timeout_sec(src._flush_timeout_sec), _shutting_down(false) {
	ignoreSigpipe();
}

//	Destructor
ServerConfig::~ServerConfig(){
	try {
		shutdown("Server shutdown");
	}
	catch (...) {
	}
}

//	Transport interface
ClientIdentity	ServerConfig::returnIdentity(irc::ClientId id) const {
	(void)id;
	return(ClientIdentity());
}

void	ServerConfig::requestDisconnection(ClientId id, const std::string &reason, bool after_flush) {
	if (_shutting_down)
		return ;
	netClientRecord	*rec = getClientById(id);
	if (rec == NULL || rec->marked_for_cleanup)
		return ;
	if(!after_flush || !rec->hasPendingOutput()) {
		rec->in_buf.clear();
		rec->out_buf.clear();
		rec->out_offset = 0;
		rec->close_after_flush = false;
		rec->flush_started_at = 0;
		markClientForCleanup(id, reason);
		return ;
	}
	if (rec->close_after_flush)
		return ;
	rec->close_after_flush = true;
	rec->disconnect_reason = reason;
	rec->flush_started_at = std::time(NULL);
	rec->in_buf.clear();
	for (size_t i = 1; i < _poll_fds.size(); ++i) {
		if (_poll_fds[i].fd == rec->fd) {
			_poll_fds[i].events &= ~POLLIN;
			_poll_fds[i].revents &= ~POLLIN;
			break ;
		}
	}
}

enqueueResult	ServerConfig::enqueueLine(ClientId id, const std::string &line) {
	if (_shutting_down)
		return (Rejected);
	netClientRecord	*rec = getClientById(id);
	if(rec == NULL || rec->fd < 0 || rec->marked_for_cleanup || rec->close_after_flush)
		return (Rejected);
	if(line.find('\r') != std::string::npos || line.find('\n') != std::string::npos)
		return (Rejected);
	if (!rec->hasPendingOutput()) {
		rec->out_buf.clear();
		rec->out_offset = 0;
	}
	rec->out_buf.append(line);
	rec->out_buf.append("\r\n");
	setClientWriteInterest_(rec->fd, true);
	return (Queued);
}

//	Operator

//		--/++
ServerConfig	&ServerConfig::operator++() {
	if (_client_nb == MAX_CLIENT)
		throw ServerConfig::ClientOutOfRange();
	_client_nb++;
	return (*this);
}

ServerConfig	ServerConfig::operator++(int) {
	ServerConfig	tmp(*this);
	++(*this);
	return (tmp);
}

ServerConfig	&ServerConfig::operator--() {
	if (_client_nb == 0)
		throw ServerConfig::ClientUnderFlow();
	_client_nb--;
	return (*this);
}

ServerConfig	ServerConfig::operator--(int) {
	ServerConfig	tmp(*this);
	--(*this);
	return (tmp);
}

const char	*ServerConfig::ClientOutOfRange::what() const throw() {
	return ("Error: Client ServerConfig._client_nb Out Of Range");
}

const char	*ServerConfig::ClientUnderFlow::what() const throw() {
	return ("Error: Client ServerConfig._client_nb UnderFlow");
}

const char	*ServerConfig::SocketOptionException::what() const throw() {
	return ("Error: Failed to configure listener socket options (SO_REUSEADDR / O_NONBLOCK)");
}

const char	*ServerConfig::SocketBindException::what() const throw() {
	return ("Error: Failed to bind listening address and port");
}

const char	*ServerConfig::SocketListenException::what() const throw() {
	return ("Error: Failed to listen on bound socket");
}

const char	*ServerConfig::PollException::what() const throw() {
	return ("Error: Poll readiness wait failed or poll table is empty");
}

//	Geter
int	ServerConfig::getPort() const {
	return (_port);
}

size_t	ServerConfig::getClientNb() const {
	return (_client_nb);
}

int ServerConfig::getListenFd() const {
	return (_listen_sock.get());
}

bool	ServerConfig::isListening() const {
	return (_is_listening);
}

bool	ServerConfig::isListenerReady() const {
	if (!_is_listening || !_listen_sock.isValid() || _poll_fds.empty())
		return (false);
	if (_poll_fds[0].fd != _listen_sock.get())
		return (false);
	return((_poll_fds[0].revents & POLLIN) != 0);
}

const std::vector<struct pollfd>	&ServerConfig::getPollFds()	const {
	return (_poll_fds);
}

const std::vector<int>				&ServerConfig::getClientFds() const {
	return (_client_fds);
}

irc::ClientId	ServerConfig::getLastAcceptedClientId() const {
	return (_last_accpted_id);
}

irc::ClientId	ServerConfig::getClientIdByFd(int fd) const {
	std::map<int, irc::ClientId>::const_iterator it = _fd_to_client_id.find(fd);
	if (it == _fd_to_client_id.end())
		return (0);
	return (it->second);
}

const netClientRecord	*ServerConfig::getClientById(irc::ClientId id) const {
	std::map<irc::ClientId, netClientRecord>::const_iterator it = _clients_by_id.find(id);
	if (it == _clients_by_id.end())
		return (NULL);
	return (&it->second);
}

netClientRecord	*ServerConfig::getClientById(irc::ClientId id) {
	std::map<irc::ClientId, netClientRecord>::iterator it = _clients_by_id.find(id);
	if (it == _clients_by_id.end())
		return (NULL);
	return (&it->second);
}

const netClientRecord	*ServerConfig::getClientByFd(int fd) const{
	irc::ClientId id = getClientIdByFd(fd);
	if (id == 0)
		return (NULL);
	return (getClientById(id));
}

bool	ServerConfig::isIdExhausted() const {
	return (_id_exhausted);
}

protocol	*ServerConfig::getProtocol() const{
	return (_protocol);
}

ChannelService	*ServerConfig::getChannelService() const {
	return (_channel_service);
}

int	ServerConfig::getEffectivePollTimeout(int default_timeout_ms) const {
	if (hasPendingCompleteLines())
		return (0);
	if (hasClosingClients()) {
		if (default_timeout_ms < 0 || default_timeout_ms > FLUSH_POLL_TIMEOUT_MS)
			return (FLUSH_POLL_TIMEOUT_MS);
	}
	return (default_timeout_ms);
}

size_t	ServerConfig::getLineBudget() const {
	return (_line_budget);
}

std::string	ServerConfig::getClientDisconnectReason(ClientId id) const {
	const netClientRecord	*rec = getClientById(id);
	if (rec == NULL)
		return ("");
	return (rec->disconnect_reason);
}

size_t	ServerConfig::getClientPendingOutputSize(ClientId id) const {
	const netClientRecord	*rec = getClientById(id);
	if (rec == NULL)
		return (0);
	return (rec->pendingOutputSize());
}

size_t	ServerConfig::getClientOutputOffset(ClientId id) const {
	const netClientRecord	*rec = getClientById(id);
	if (rec == NULL)
		return (0);
	return (rec->out_offset);
}

std::time_t	ServerConfig::getFlushTimeoutSec() const {
	return (_flush_timeout_sec);
}

void	ServerConfig::setFlushTimeoutSec(std::time_t timeout_sec) {
	_flush_timeout_sec = (timeout_sec < 0) ? 0 : timeout_sec;
}



bool	ServerConfig::isClientReadable(ClientId id) const {
	const netClientRecord	*rec = getClientById(id);
	if (rec == NULL || rec->fd < 0 || rec->marked_for_cleanup || rec->close_after_flush)
		return (false);
	for (size_t i = 1; i < _poll_fds.size(); ++i) {
		if (_poll_fds[i].fd == rec->fd)
			return ((_poll_fds[i].revents & POLLIN) != 0);
	}
	return (false);
}

bool	ServerConfig::isFdReadable(int fd) const {
	ClientId	id = getClientIdByFd(fd);
	if (id == 0)
		return (false);
	return (isClientReadable(id));
}

bool	ServerConfig::isClientWriteInterestEnabled(ClientId id) const {
	const netClientRecord	*rec = getClientById(id);
	if (rec == NULL || rec->fd < 0)
		return (false);
	for (size_t i = 1; i < _poll_fds.size(); ++i) {
		if (_poll_fds[i].fd == rec->fd)
			return ((_poll_fds[i].events & POLLOUT) != 0);
	}
	return (false);
}

bool	ServerConfig::isClientClosingAfterFlush(ClientId id) const {
	const netClientRecord	*rec = getClientById(id);
	if (rec == NULL)
		return (false);
	return (rec->close_after_flush);

}

bool	ServerConfig::isStopRequested() {
	return (g_stop_requested != 0);
}

bool	ServerConfig::isShuttingDown() const {
	return (_shutting_down);
}

bool	ServerConfig::isClientMarkedForCleanup(ClientId id) const {
	const netClientRecord	*rec=getClientById(id);
	if (rec == NULL)
		return (false);
	return(rec->marked_for_cleanup);
}

bool	ServerConfig::hasClientPendingOutput(ClientId id) const {
	const netClientRecord	*rec = getClientById(id);
	if (rec == NULL)
		return (false);
	return (rec->hasPendingOutput());
}

bool	ServerConfig::hasClientCompleteLine(ClientId id) const {
	if (_shutting_down)
		return (false);
	const netClientRecord	*rec = getClientById(id);
	if (rec == NULL || rec->marked_for_cleanup || rec->close_after_flush)
		return (false);
	return (rec->in_buf.find('\n') != std::string::npos);
}

bool	ServerConfig::hasPendingCompleteLines() const {
	if (_shutting_down)
		return (false);
	for (std::map<ClientId, netClientRecord>::const_iterator it = _clients_by_id.begin();
		it != _clients_by_id.end(); ++it)
		{
			if (!it->second.marked_for_cleanup && !it->second.close_after_flush
					&& it->second.in_buf.find('\n') != std::string::npos)
				return (true);
		}
	return(false);
}

bool	ServerConfig::hasClosingClients() const {
	if (_shutting_down)
		return (false);
	for (std::map<ClientId, netClientRecord>::const_iterator it = _clients_by_id.begin();
			it != _clients_by_id.end(); ++it)
	{
		if (it->second.close_after_flush && !it->second.marked_for_cleanup)
			return (true);
	}
	return (false);
}

//	Seter
void	ServerConfig::ignoreSigpipe() {
	struct sigaction	sa;
	std::memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	::sigaction(SIGPIPE, &sa, NULL);
}

bool	ServerConfig::installShutdownSignals() {
	g_stop_requested = 0;
	struct sigaction	sa;
	std::memset(&sa, 0, sizeof(sa));
	sa.sa_handler = irc_handle_shutdown_signal;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (::sigaction(SIGINT, &sa, NULL) < 0)
		return (false);
	if (::sigaction(SIGTERM, &sa, NULL) < 0)
		return (false);
	return (true);
}

void	ServerConfig::resetStopFlag() {
	g_stop_requested = 0;
}



size_t	ServerConfig::enforceFlushCloseBounds() {
	if (_shutting_down)
		return (0);
	return (enforceFlushCloseBounds(std::time(NULL)));
}

size_t	ServerConfig::enforceFlushCloseBounds(std::time_t now) {
	size_t	expired = 0;
	for (std::map<ClientId, netClientRecord>::iterator it = _clients_by_id.begin();
			it != _clients_by_id.end(); ++it)
	{
		netClientRecord	&rec = it->second;
		if (!rec.close_after_flush || rec.marked_for_cleanup)
			continue ;
		if (!rec.hasPendingOutput()) {
			markClientForCleanup(it->first, rec.disconnect_reason);
			++expired;
			continue ;
		}
		bool	timed_out = (now == static_cast<std::time_t>(-1))
						|| (rec.flush_started_at == static_cast<std::time_t>(-1))
						|| (now < rec.flush_started_at)
						|| ((now - rec.flush_started_at) >= _flush_timeout_sec);
		if (timed_out) {
			rec.out_buf.clear();
			rec.out_offset = 0;
			std::string	reason = rec.disconnect_reason.empty()
							? "Flush-before-close timeout exceeded"
							: rec.disconnect_reason + " (flush timeout)";
			markClientForCleanup(it->first, reason);
			++expired;
		}
	}
	return (expired);
}

void	ServerConfig::setProtocol(protocol *proto) {
	_protocol = proto;
}

void	ServerConfig::createListenerSocket(int domain) {
	closeListener();
	_listen_sock.openTcp(domain);
}

void	ServerConfig::configureListenerSocket() {
	if (!_listen_sock.isValid())
		throw ServerConfig::SocketOptionException();
	int fd = _listen_sock.get();
	int opt = 1;
	if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		closeListener();
		throw ServerConfig::SocketOptionException();
	}
	if (::fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
		closeListener();
		throw ServerConfig::SocketOptionException();
	}
}

void	ServerConfig::bindListenerSocket(in_addr_t bind_addr) {
	if (!_listen_sock.isValid())
		throw ServerConfig::SocketBindException();
	struct sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(bind_addr);
	addr.sin_port = htons(static_cast<uint16_t>(_port));
	 if (::bind(_listen_sock.get(), reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0) {
		closeListener();
		throw ServerConfig::SocketBindException();
	 }
}

void	ServerConfig::startListening(int backlog) {
	if (!_listen_sock.isValid() || backlog <= 0) {
		closeListener();
		throw ServerConfig::SocketListenException();
	}
	if (::listen(_listen_sock.get(), backlog)) {
		closeListener();
		throw ServerConfig::SocketListenException();
	}
	_is_listening = true;
}

void	ServerConfig::initListener(in_addr_t bind_addr, int backlog) {
	ignoreSigpipe();
	createListenerSocket(AF_INET);
	configureListenerSocket();
	bindListenerSocket(bind_addr);
	startListening(backlog);
	registerListenerInPoll_();
}

void	ServerConfig::closeListener() {
	shutdown("Server shutdown");
}

void	ServerConfig::registerListenerInPoll_() {
	if (!_listen_sock.isValid() || !_is_listening)
		throw ServerConfig::PollException();
	_poll_fds.clear();
	_pending_removals.clear();
	struct pollfd pfd;
	pfd.fd = _listen_sock.get();
	pfd.events = POLLIN;
	pfd.revents = 0;
	_last_accpted_id = 0;
	_client_nb = 0;
	_poll_fds.push_back(pfd);
}

int	ServerConfig::waitForEvents(int timeout_ms) {
	if (_shutting_down)
		return (0);
	if (_poll_fds.empty())
		throw ServerConfig::PollException();
	for (size_t i = 0; i < _poll_fds.size(); ++i) {
		_poll_fds[i].revents = 0;
	}
	if (isStopRequested())
		return (0);
	int ready = poll(&_poll_fds[0], _poll_fds.size(), timeout_ms);
	if (ready < 0) {
		if (isStopRequested())
			return (0);
		throw ServerConfig::PollException();
	}
	return (ready);
}

int	ServerConfig::acceptReadyClient() {
	if (_shutting_down || !isListenerReady())
		return (-1);
	_poll_fds[0].revents &= ~POLLIN;
	int	raw_fd = ::accept(_listen_sock.get(), NULL, NULL);
	if (raw_fd < 0)
		return (-1);
	ScopedSocket	client_sock(raw_fd);
	if (::fcntl(client_sock.get(), F_SETFL, O_NONBLOCK) < 0)
		return (-1);
	if (_client_nb >= MAX_CLIENT)
		return (-1);
	if (_id_exhausted || _next_client_id == 0 || _clients_by_id.find(_next_client_id) != _clients_by_id.end())
		return (-1);
	irc::ClientId	assigned_id = _next_client_id;
	int	fd = client_sock.get();
	_clients_by_id.insert(std::make_pair(assigned_id, netClientRecord(assigned_id, fd)));
	_fd_to_client_id.insert(std::make_pair(fd, assigned_id));
	if (_next_client_id == ULONG_MAX)
		_id_exhausted = true;
	else
		++_next_client_id;
	int owned_fd = client_sock.release();
	_client_fds.push_back(owned_fd);

	struct pollfd	client_pfd;
	client_pfd.fd = owned_fd;
	client_pfd.events = POLLIN;
	client_pfd.revents = 0;
	_poll_fds.push_back(client_pfd);
	
	_last_accpted_id = assigned_id;
	++_client_nb;
	if (_protocol != NULL)
		_protocol->connected(assigned_id);
	return (owned_fd);
}

bool	ServerConfig::removeClientById(ClientId id) {
	return (disconnectClient(id, ""));
}

bool	ServerConfig::removeClientByFd(int fd) {
	irc::ClientId id = getClientIdByFd(fd);
	if (id == 0)
		return (false);
	return (disconnectClient(id, ""));
}

void	ServerConfig::setNextClientId(irc::ClientId next_id) {
	if (next_id == 0) {
		_id_exhausted = true;
		return ;
	}
	if (!_id_exhausted && next_id >= _next_client_id)
		_next_client_id = next_id;
}

ssize_t	ServerConfig::readReadyClient(ClientId id) {
	if (_shutting_down)
		return (-1);
	netClientRecord	*rec = getClientById(id);
	if (rec == NULL || rec->fd < 0 || rec->marked_for_cleanup || rec->close_after_flush)
		return (-1);
	size_t 	poll_index = 0;
	bool	found_ready = false;
	for (size_t i = 1; i < _poll_fds.size(); ++i ) {
		if (_poll_fds[i].fd == rec->fd) {
			if ((_poll_fds[i].revents & POLLIN) == 0)
				return (-1);
			poll_index = i;
			found_ready = true;
			break;
		}
	}
	if (!found_ready)
		return (-1);
	_poll_fds[poll_index].revents &= ~POLLIN;
	char	buf[CLIENT_READ_BUF_SIZE];
	ssize_t	n = ::recv(rec->fd, buf, sizeof(buf), 0);
	return (handleRecvResult(id, buf, n));
}

ssize_t	ServerConfig::readReadyClientByFd(int fd) {
	ClientId	id = getClientIdByFd(fd);
	if (id == 0)
		return (-1);
	return (readReadyClient(id));
}

size_t	ServerConfig::readAllReadyClients() {
	if (_shutting_down)
		return (0);
	size_t				read_cont = 0;
	std::vector<int>	fds_snapshot(_client_fds);
	for (size_t i = 0; i < fds_snapshot.size(); ++i) {
		ClientId	id = getClientIdByFd(fds_snapshot[i]);
		if (id != 0 && isClientReadable(id))
			if (readReadyClient(id) > 0)
				++read_cont;
	}
	return (read_cont);
}

ssize_t	ServerConfig::writeReadyClient(ClientId id) {
	if (_shutting_down)
		return (-1);
	netClientRecord	*rec = getClientById(id);
	if (rec == NULL || rec->fd < 0 || rec->marked_for_cleanup)
		return (-1);
	if (!rec->hasPendingOutput()) {
		setClientWriteInterest_(rec->fd, false);
		if (rec->close_after_flush)
			markClientForCleanup(id, rec->disconnect_reason);
		return (0);
	}
	size_t	poll_idx = 0;
	bool	found_ready = false;
	for (size_t i = 1; i < _poll_fds.size(); ++i) {
		if (_poll_fds[i].fd == rec->fd) {
			if ((_poll_fds[i].revents & POLLOUT) == 0)
				return (-1);
			poll_idx = i;
			found_ready = true;
			break ;
		}
	}
	if (!found_ready)
		return (-1);
	_poll_fds[poll_idx].revents &= ~POLLOUT;
	const char	*data_ptr = rec->out_buf.data() + rec->out_offset;
	size_t		remaining = rec->out_buf.size() - rec->out_offset;
	ssize_t		n = ::send(rec->fd, data_ptr, remaining, IRC_SEND_FLAGS);
	return (handleSendResult(id, n));
}

ssize_t	ServerConfig::writeReadyClientByFd(int fd) {
	ClientId	id = getClientIdByFd(fd);
	if (id == 0)
		return (-1);
	return (writeReadyClient(id));
}

size_t	ServerConfig::writeAllReadyClients() {
	if (_shutting_down)
		return (0);
	size_t	write_count = 0;
	for (size_t i = 1; i < _poll_fds.size(); ++i) {
		if ((_poll_fds[i].revents & POLLOUT) != 0) {
			ClientId	id = getClientIdByFd(_poll_fds[i].fd);
			if (id != 0 && writeReadyClient(id) > 0)
				++write_count;
		}
	}
	return (write_count);
}

size_t	ServerConfig::markErroredClientsForCleanup() {
	size_t	marked = 0;
	for (size_t i = 1; i < _poll_fds.size(); ++i) {
		short	rev = _poll_fds[i].revents;
		if ((rev & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
			ClientId	id = getClientIdByFd(_poll_fds[i].fd);
			if (id != 0 && !isClientMarkedForCleanup(id)) {
				markClientForCleanup(id, "Poll hangup or error");
				++marked;
			}
			_poll_fds[i].revents = 0;
		}
	}
	return (marked);
}


void	ServerConfig::markClientForCleanup(ClientId id, const std::string &reason) {
	if (_shutting_down)
		return ;
	netClientRecord	*rec = getClientById(id);
	if (rec == NULL || rec->marked_for_cleanup || rec->disconnecting)
		return ;
	if (!rec->marked_for_cleanup) {
		rec->marked_for_cleanup = true;
		rec->disconnect_reason = reason;
		_pending_removals.push_back(id);
	}
	for (size_t i = 1; i < _poll_fds.size(); ++i) {
		if (_poll_fds[i].fd == rec->fd) {
			_poll_fds[i].events = 0;
			_poll_fds[i].revents = 0;
			break;
		}
	}
}

ssize_t	ServerConfig::handleRecvResult(ClientId id, const char *buf, ssize_t n) {
	netClientRecord	*rec = getClientById(id);
	if (rec == NULL || rec->close_after_flush)
		return (-1);
	if (n > 0){
		if (buf != NULL)
			rec->in_buf.append(buf, static_cast<size_t>(n));
		return (n);
	}
	if (n == 0) {
		markClientForCleanup(id, "Peer closed connection");
		return (0);
	}
	markClientForCleanup(id, "Fatal read failure");
	return (n);
}

ssize_t	ServerConfig::injectRecvResult(ClientId id, ssize_t simulated_ret) {
	return (handleRecvResult(id, NULL, simulated_ret));
}

 ssize_t	ServerConfig::handleSendResult(ClientId id, ssize_t n) {
	netClientRecord	*rec = getClientById(id);
	if (rec == NULL)
		return (-1);
	if (n > 0) {
		size_t	sent = static_cast<size_t>(n);
		size_t	remaining = rec->pendingOutputSize();
		if (sent > remaining)
			sent = remaining;
		rec->out_offset += sent;
		if (!rec->hasPendingOutput()) {
			rec->out_buf.clear();
			rec->out_offset = 0;
			setClientWriteInterest_(rec->fd, false);
			if (rec->close_after_flush)
				markClientForCleanup(id, rec->disconnect_reason);
		}
		return (n);
	}
	markClientForCleanup(id, "Fatal write failure");
	return (n);
 }

ssize_t	ServerConfig::injectSendResult(ClientId id, ssize_t simulated_ret) {
	return (handleSendResult(id, simulated_ret));
}

size_t	ServerConfig::cleanupMarkedClients() {
	if (_shutting_down)
		return (0);
	size_t					cleaned = 0;
	std::vector<ClientId>	to_rm;
	to_rm.swap(_pending_removals);
	for(std::map<ClientId, netClientRecord>::const_iterator it = _clients_by_id.begin();
		it != _clients_by_id.end(); ++it)
		{
			if (it->second.marked_for_cleanup && std::find(to_rm.begin(), to_rm.end(), it->first) == to_rm.end())
				to_rm.push_back(it->first); 
	}
	for (size_t i = 0; i < to_rm.size(); ++i) {
		if (disconnectClient(to_rm[i], ""))
			++cleaned;
	}
	return (cleaned);
}

void	ServerConfig::setLineBudget(size_t budget) {
	_line_budget = (budget == 0) ? 1 : budget;
}

bool	ServerConfig::extractNextClientLine(ClientId id, std::string &out_line) {
	if (_shutting_down)
		return (false);
	netClientRecord	*rec = getClientById(id);
	if(rec == NULL || rec->marked_for_cleanup || rec->close_after_flush)
		return (false);
	std::string::size_type	lf_pos = rec->in_buf.find('\n');
	if (lf_pos == std::string::npos)
		return (false);
	std::string::size_type	content_len = lf_pos;
	if (content_len > 0 && rec->in_buf[content_len - 1] == '\r')
		--content_len;	
	out_line.assign(rec->in_buf, 0, content_len);
	rec->in_buf.erase(0, lf_pos + 1);
	return (true);
}

size_t	ServerConfig::dispatchClientLines(ClientId id) {
	if (_shutting_down)
		return (0);
	size_t		dispatched = 0;
	std::string	line;
	while (dispatched < _line_budget) {
		if (_shutting_down)
			break;
		netClientRecord	*rec =	 getClientById(id);
		if (rec == NULL || rec->marked_for_cleanup || rec->close_after_flush)
			break ;
		if (!extractNextClientLine(id, line))
			break ;
		++dispatched;
		if (_protocol != NULL)
			_protocol->online(id, line);
	}
	return (dispatched);
}

size_t	ServerConfig::dispatchAllReadyLines() {
	if (_shutting_down)
		return (0);
	size_t				total_dispached = 0;
	std::vector<int>	fds_snaphot(_client_fds);
	for (size_t i = 0; i < fds_snaphot.size(); ++i) {
		if (_shutting_down)
			break;
		ClientId		id = getClientIdByFd(fds_snaphot[i]);
		if (id != 0 && hasClientCompleteLine(id))
			total_dispached += dispatchClientLines(id); 
	}
	return (total_dispached);
}

void	ServerConfig::setChannelService(ChannelService *chan_s) {
	_channel_service = chan_s;
}

bool	ServerConfig::disconnectClient(ClientId id, const std::string &reason) {
	std::map<ClientId, netClientRecord>::iterator it = _clients_by_id.find(id);
	if (it == _clients_by_id.end() || it->second.disconnecting)
		return (false);
	it->second.disconnecting = true;
	it->second.marked_for_cleanup = true;
	it->second.close_after_flush = false;
	it->second.flush_started_at = 0;
	if (it->second.disconnect_reason.empty() && !reason.empty())
		it->second.disconnect_reason = reason;
	const std::string	final_reason = it->second.disconnect_reason;
	const int			fd = it->second.fd;
	if (fd >= 0) {
		for (size_t i = 1; i < _poll_fds.size(); i++) {
			if (_poll_fds[i].fd == fd) {
				_poll_fds[i].events = 0;
				_poll_fds[i].revents = 0;
				break;
			}
		}
	}

	if (_protocol != NULL)
		_protocol->disconnected(id, final_reason);
	if (_channel_service != NULL)
		_channel_service->removeClient(id);
	
	it = _clients_by_id.find(id);
	if (it != _clients_by_id.end()) {
		it->second.in_buf.clear();
		it->second.out_buf.clear();
		it->second.disconnect_reason.clear();
		_clients_by_id.erase(it);
	}

	if (fd >= 0) {
		_fd_to_client_id.erase(fd);
		std::vector<int>::iterator vec_it = std::find(_client_fds.begin(), _client_fds.end(), fd);
		if (vec_it != _client_fds.end())
			_client_fds.erase(vec_it);
		for (size_t i = 1; i < _poll_fds.size(); ++i) {
			if (_poll_fds[i].fd == fd) {
				_poll_fds.erase(_poll_fds.begin() + i);
				break;
			}
		}
		::close(fd);
	}
	if (_client_nb > 0)
		--_client_nb;
	return(true);
}

void	ServerConfig::shutdown(const std::string &reason) {
	if (_shutting_down)
		return ;
	ScopedShutdownGuard	guard(_shutting_down);
	stopAdmission_();
	_pending_removals.clear();
	std::vector<ClientId>	client_ids;
	client_ids.reserve(_clients_by_id.size());
	for (std::map<ClientId, netClientRecord>::const_iterator it = _clients_by_id.begin();
			it != _clients_by_id.end(); ++it)
		{
			 client_ids.push_back(it->first);
	}
	const std::string	final_reason = reason.empty() ? "Server shutdown" : reason;
	for (size_t i = 0; i < client_ids.size(); ++i) {
		disconnectClient(client_ids[i], final_reason);
	}
	for (size_t i = 0; i < _client_fds.size(); ++i) {
		if (_client_fds[i] >= 0)
			::close(_client_fds[i]);
	}
	_client_fds.clear();
	_clients_by_id.clear();
	_fd_to_client_id.clear();
	_pending_removals.clear();
	_poll_fds.clear();
	_is_listening = false;
	_listen_sock.reset();
	_client_nb = 0;
	_last_accpted_id = 0;
}

//	Other
std::ostream	&operator<<(std::ostream &os, const ServerConfig &src) {
	os << "ircserv: Port: " << src.getPort()
	   << " | Listening: " << (src.isListening() ? "yes" : "no")
	   << " | Connected CLient: " << src.getClientNb();
	return (os);
}
//	Debug
std::string ServerConfig::getPSW() const {
	return (_psw);
}

}	//	namespace irc
