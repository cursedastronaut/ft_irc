#ifndef SERVERCONFIG_HPP
# define SERVERCONFIG_HPP

# include <poll.h>
# include <iostream>
# include <exception>
# include <cstdlib>
# include <climits>
# include <string>
# include <algorithm>

# include "settings.hpp"
# include "net/parsers.hpp"


class ServerConfig
{
private:
	const int			_port;		//	Port connection
	const std::string	_psw;		//	Connection password
	size_t				_client_nb;	//	Total number of client on the server
	
	ServerConfig();				//	Can't creat an default ircserv
	ServerConfig	&operator=(const ServerConfig &src);	//	Can't assigne const port or const psw
	
	public:
	//	Constructor
	ServerConfig(int irc_port, std::string psw);
	ServerConfig(const ServerConfig &src);
	
	//	Destructor
	~ServerConfig();
	
	//	Operator

	//		++/--
	ServerConfig	&operator++();
	ServerConfig	operator++(int);
	ServerConfig	&operator--();
	ServerConfig	operator--(int);

	//	Exception
	class ClientUnderFlow : public std::exception {
		public:
			virtual const char* what() const throw();
	};
	
	class ClientOutOfRange : public std::exception {
		public:
			virtual const char* what() const throw();
	};

	//	Geter
	int			getPort() const;
	size_t		getClientNb() const;

	//	Debug
	std::string	getPSW() const;		//	Debug Use Only (Display raw _psw !!!)
};

std::ostream	&operator<<(std::ostream &os, const ServerConfig &src);

#endif	// ServerConfig_HPP