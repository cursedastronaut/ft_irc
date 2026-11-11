#include "net/ServerConfig.hpp"

//	Constructor
ServerConfig::ServerConfig(int irc_port, std::string psw)
	: _port(irc_port), _psw(psw), _client_nb(0), _listen_sock(),
		_is_listening(false), _poll_fds(), _client_fds(),
		_next_client_id(1), _id_exhausted(false), _last_accpted_id(0),
		_clients_by_id(), _fd_to_client_id() {

}

ServerConfig::ServerConfig(const ServerConfig &src)
	: _port(src._port), _psw(src._psw), _client_nb(src._client_nb),
		_listen_sock(), _is_listening(false), _poll_fds(), _client_fds(),
		_next_client_id(src._next_client_id), _id_exhausted(src._id_exhausted),
		_last_accpted_id(src._last_accpted_id), _clients_by_id(src._clients_by_id),
		_fd_to_client_id(src._fd_to_client_id) {

}

//	Destructor
ServerConfig::~ServerConfig(){
	closeListener();
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

const ClientRecord	*ServerConfig::getClientById(irc::ClientId id) const {
	std::map<irc::ClientId, ClientRecord>::const_iterator it = _clients_by_id.find(id);
	if (it == _clients_by_id.end())
		return (NULL);
	return (&it->second);
}

ClientRecord	*ServerConfig::getClientById(irc::ClientId id) {
	std::map<irc::ClientId, ClientRecord>::iterator it = _clients_by_id.find(id);
	if (it == _clients_by_id.end())
		return (NULL);
	return (&it->second);
}

const ClientRecord	*ServerConfig::getClientByFd(int fd) const{
	irc::ClientId id = getClientIdByFd(fd);
	if (id == 0)
		return (NULL);
	return (getClientById(id));
}

bool	ServerConfig::isIdExhausted() const {
	return (_id_exhausted);
}

//	Seter
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
	createListenerSocket(AF_INET);
	configureListenerSocket();
	bindListenerSocket(bind_addr);
	startListening(backlog);
	registerListenerInPoll_();
}

void	ServerConfig::closeListener() {
	for (size_t i = 0; i < _client_fds.size(); ++i) {
		if (_client_fds[i] >= 0)
			::close(_client_fds[i]);
	}
	_client_fds.clear();
	_clients_by_id.clear();
	_fd_to_client_id.clear();
	_poll_fds.clear();
	_is_listening = false;
	_listen_sock.reset();
	_client_nb = 0;
}

void	ServerConfig::registerListenerInPoll_() {
	if (!_listen_sock.isValid() || !_is_listening)
		throw ServerConfig::PollException();
	_poll_fds.clear();
	struct pollfd pfd;
	pfd.fd = _listen_sock.get();
	pfd.events = POLLIN;
	pfd.revents = 0;
	_last_accpted_id = 0;
	_client_nb = 0;
	_poll_fds.push_back(pfd);
}

int	ServerConfig::waitForEvents(int timeout_ms) {
	if (_poll_fds.empty())
		throw ServerConfig::PollException();
	for (size_t i = 0; i < _poll_fds.size(); ++i) {
		_poll_fds[i].revents = 0;
	}

	int ready = poll(&_poll_fds[0], _poll_fds.size(), timeout_ms);
	if (ready < 0)
		throw ServerConfig::PollException();
	return (ready);
}

int	ServerConfig::acceptReadyClient() {
	if (!isListenerReady())
		return (-1);
	_poll_fds[0].revents &= ~POLLIN;
	int	raw_fd = ::accept(_listen_sock.get(), NULL, NULL);
	if (raw_fd < 0)
		return (-1);
	ScopedSocket	client_sock(raw_fd);
	if (fcntl(client_sock.get(), F_SETFL, O_NONBLOCK) < 0)
		return (-1);
	if (_client_nb >= MAX_CLIENT)
		return (-1);
	if (_id_exhausted || _next_client_id == 0 || _clients_by_id.find(_next_client_id) != _clients_by_id.end())
		return (-1);
	irc::ClientId	assigned_id = _next_client_id;
	int	fd = client_sock.get();
	_clients_by_id.insert(std::make_pair(assigned_id, ClientRecord(assigned_id, fd)));
	_fd_to_client_id.insert(std::make_pair(fd, assigned_id));
	if (_next_client_id == ULONG_MAX)
		_id_exhausted = true;
	else
		++_next_client_id;
	int owned_fd = client_sock.release();
	_client_fds.push_back(owned_fd);
	_last_accpted_id = assigned_id;
	++_client_nb;
	return (owned_fd);
}

bool	ServerConfig::removeClientById(irc::ClientId id) {
	std::map<irc::ClientId, ClientRecord>::iterator it = _clients_by_id.find(id);
	if (it == _clients_by_id.end())
		return (false);
	int fd = it->second.fd;
	_fd_to_client_id.erase(fd);
	_clients_by_id.erase(it);
	std::vector<int>::iterator vec_it = std::find(_client_fds.begin(), _client_fds.end(), fd);
	if (vec_it != _client_fds.end())
		_client_fds.erase(vec_it);
	if (fd >= 0)
		::close(fd);
	if (_client_nb > 0)
		--_client_nb;
	return (true);
}

bool	ServerConfig::removeClientByFd(int fd) {
	irc::ClientId id = getClientIdByFd(fd);
	if (id == 0)
		return (false);
	return (removeClientById(id));
}

void	ServerConfig::setNextClientId(irc::ClientId next_id) {
	if (next_id == 0) {
		_id_exhausted = true;
		return ;
	}
	if (!_id_exhausted && next_id >= _next_client_id)
		_next_client_id = next_id;
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
