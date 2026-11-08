#include "net/ServerConfig.hpp"

//	Constructor
ServerConfig::ServerConfig(int irc_port, std::string psw) : _port(irc_port), _psw(psw), _client_nb(0) {

}

ServerConfig::ServerConfig(const ServerConfig &src) : _port(src._port), _psw(src._psw), _client_nb(src._client_nb) {

}

//	Destructor
ServerConfig::~ServerConfig(){

}

//	Operator

// ServerConfig	&ServerConfig::operator=(const ServerConfig &src) {
// 	if (this != &src) {
// 		_client_nb = src._client_nb;
// 	}
// 	return (*this);
// }

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

//	Geter
int	ServerConfig::getPort() const {
	return (_port);
}

size_t	ServerConfig::getClientNb() const {
	return (_client_nb);
}


std::ostream	&operator<<(std::ostream &os, const ServerConfig &src) {
	os << "ircserv: Port: " << src.getPort() << " | Connected CLient: " << src.getClientNb();
	return (os);
}

//	Debug
std::string ServerConfig::getPSW() const {
	return (_psw);
}