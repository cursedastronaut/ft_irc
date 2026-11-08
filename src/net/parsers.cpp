#include "net/parsers.hpp"

//	Exception
const char *ParserPortEmptyPort::what() const throw() {
	return ("Error: Port empty: must be filled with value between "
			TOSTRING(MIN_PORT) " and " TOSTRING(MAX_PORT));
}

const char	*ParserPortIvalideFormat::what() const throw() {
	return ("Error: Port invalide format: must be filled with value between "
			TOSTRING(MIN_PORT) " and " TOSTRING(MAX_PORT));;
}

const char	*ParserPortOverFlow::what() const throw() {
	return ("Error: Port Overflow: must be filled with value between "
			TOSTRING(MIN_PORT) " and " TOSTRING(MAX_PORT));
}

const char	*ParserPortOutOfRange::what() const throw() {
	return ("Error: Port Out of range: must be filled with value between "
			TOSTRING(MIN_PORT) " and " TOSTRING(MAX_PORT));
}

const char	*ParserPSWEmpty::what() const throw() {
	return ("Error: Empty password detected");
}


int	port_parser(const char *str_port) {
	if (!str_port)
		throw ParserPortEmptyPort();
	std::string s_port = str_port;
	if (s_port.empty())
		throw ParserPortEmptyPort();
	for (size_t i = 0; i < s_port.size(); i++) {
		if (!isdigit(s_port[i]))
			throw ParserPortIvalideFormat();
	}
	int port;
	std::istringstream iss(s_port);
	if (!(iss >> port)) {
		throw ParserPortOverFlow();
	}
	if (port < MIN_PORT || MAX_PORT < port)
		throw ParserPortOutOfRange();
	return (port);
}

std::string	psw_parser(const char *str_psw) {
	if (!str_psw || !*str_psw)
		throw ParserPSWEmpty();
	return(std::string(str_psw));
}