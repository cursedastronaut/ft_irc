#include "net/ScopedSocket.hpp"

ScopedSocket::ScopedSocket(): _fd(-1) {

}

ScopedSocket::ScopedSocket(int fd): _fd(fd) {
    if (_fd < 0)
        throw ScopedSocket::SocketCreationException();
}

ScopedSocket::~ScopedSocket() {
	reset();
}

int		ScopedSocket::get() const {
	return (_fd);
}

bool	ScopedSocket::isValid() const {
	return (_fd >= 0);
}

void	ScopedSocket::openTcp(int domain) {
	if (domain != AF_INET && domain != AF_INET6)
		throw ScopedSocket::InvalidFamilyException();
	int new_fd = socket(domain, SOCK_STREAM, 0);
	if (new_fd < 0)
		throw ScopedSocket::SocketCreationException();
	reset(new_fd);
}

int		ScopedSocket::release() {
	int	tmp = _fd;
	_fd = -1;
	return (tmp);
}

void	ScopedSocket::reset(int new_fd) {
	if (_fd >= 0 && _fd != new_fd)
		::close(_fd);
	_fd = new_fd;
}

void	ScopedSocket::swap(ScopedSocket &other) {
	int	tmp = _fd;
	_fd = other._fd;
	other._fd = tmp;
}

const char	*ScopedSocket::SocketCreationException::what() const throw() {
	return("Error: Failed to creat TCP steam socket");
}

const char	*ScopedSocket::InvalidFamilyException::what() const throw() {
	return("Error: Unsupported addresse familly (must be AF_INET or AF_INET6)");
}