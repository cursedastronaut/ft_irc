#ifndef SERVERCONFIG_HPP
# define SERVERCONFIG_HPP

# include <poll.h>
# include <fcntl.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <sys/socket.h>
# include <unistd.h>
# include <iostream>
# include <exception>
# include <cstdlib>
# include <climits>
# include <string>
# include <algorithm>
# include <cstring>
# include <vector>
# include <map>

# include "settings.hpp"
# include "net/parsers.hpp"
# include "net/ScopedSocket.hpp"
# include "net/ClientRecord.hpp"

class ServerConfig
{
private:
	const int								_port;		//	Port connection
	const std::string						_psw;		//	Connection password
	size_t									_client_nb;	//	Total number of client on the server
	ScopedSocket							_listen_sock;
	bool									_is_listening;
	std::vector<struct pollfd>				_poll_fds;
	std::vector<int>						_client_fds;
	irc::ClientId							_next_client_id;
	bool									_id_exhausted;
	irc::ClientId							_last_accpted_id;
	std::map<irc::ClientId, ClientRecord>	_clients_by_id;
	std::map<int, irc::ClientId>			_fd_to_client_id;

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

	class SocketOptionException: public std::exception {
		public:
			virtual const char *what() const throw();
	};

	class SocketBindException: public std::exception {
		public:
			virtual const char *what() const throw();
	};

	class SocketListenException: public std::exception {
		public:
			virtual const char *what() const throw();
	};

	class PollException: public std::exception {
		public:
			virtual const char *what() const throw();
	};

	//	Geter
	int									getPort() const;
	size_t								getClientNb() const;
	int									getListenFd() const;
	bool								isListening() const;
	bool								isListenerReady() const;
	const std::vector<struct pollfd>	&getPollFds() const;
	const std::vector<int>				&getClientFds() const;
	irc::ClientId						getLastAcceptedClientId() const;
	irc::ClientId						getClientIdByFd(int fd) const;
	const ClientRecord					*getClientById(irc::ClientId id) const;
	ClientRecord						*getClientById(irc::ClientId id);
	const ClientRecord					*getClientByFd(int fd) const;
	bool								isIdExhausted() const;
	
	//	Seter
	void		createListenerSocket(int domain = AF_INET);
	void		configureListenerSocket();
	void		bindListenerSocket(in_addr_t bind_addr = INADDR_ANY);
	void		startListening(int backlog = LISTEN_BACKLOG);
	void		initListener(in_addr_t bind = INADDR_ANY, int Back_log = LISTEN_BACKLOG);
	void		closeListener();
	void		registerListenerInPoll_();
	int			waitForEvents(int timeout_ms = POLL_TIMEOUT_MS);
	int			acceptReadyClient();
	bool		removeClientById(irc::ClientId id);
	bool		removeClientByFd(int fd);
	void		setNextClientId(irc::ClientId next_id);

	//	Debug
	std::string	getPSW() const;		//	Debug Use Only (Display raw _psw !!!)
};

std::ostream	&operator<<(std::ostream &os, const ServerConfig &src);

#endif	// SERVERCONFIG_HPP
