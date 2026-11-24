#ifndef SERVERCONFIG_HPP
# define SERVERCONFIG_HPP

# include <poll.h>
# include <fcntl.h>
# include <csignal>
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
# include <ctime>

# include "settings.hpp"
# include "contracts/Protocol.hpp"
# include "contracts/ChannelService.hpp"
# include "net/parsers.hpp"
# include "net/ScopedSocket.hpp"
# include "net/ClientRecord.hpp"
# include "contracts/PasswordSource.hpp"

namespace irc
{

class ServerConfig : public transport, public ::PasswordSource
{
public:
	enum PollStatus {
		PollIdle = 0,
		PollReady,
		PollTimeout,
		PollStopRequested,
		PollError
	};

private:
	const int								_port;
	const std::string						_psw;
	size_t									_client_nb;
	ScopedSocket							_listen_sock;
	bool									_is_listening;
	std::vector<struct pollfd>				_poll_fds;
	std::vector<int>						_client_fds;
	irc::ClientId							_next_client_id;
	bool									_id_exhausted;
	irc::ClientId							_last_accpted_id;
	std::map<irc::ClientId, netClientRecord>	_clients_by_id;
	std::map<int, irc::ClientId>			_fd_to_client_id;
	std::vector<ClientId>					_pending_removals;
	protocol								*_protocol;
	ChannelService							*_channel_service;
	size_t									_line_budget;
	size_t									_dispatch_byte_budget;
	size_t									_read_byte_budget;
	size_t									_write_byte_budget;
	size_t									_in_buf_high_water;
	std::time_t								_flush_timeout_sec;
	bool									_shutting_down;
	PollStatus								_last_poll_status;

	ServerConfig();				//	Can't creat an default ircserv
	ServerConfig	&operator=(const ServerConfig &src);	//	Can't assigne const port or const psw
	
	void	setClientReadInterest_(int fd, bool enable);
	void	updateClientReadInterest_(ClientId id);
	void	setClientWriteInterest_(int fd, bool enable);
	void	stopAdmission_();

public:
	//	Constructor
	ServerConfig(int irc_port, std::string psw, protocol *prtoto = NULL, ChannelService *chan_s = NULL);
	ServerConfig(const ServerConfig &src);
	
	//	Destructor
	~ServerConfig();
	
	//	Transport interface
	virtual	ClientIdentity	returnIdentity(ClientId id) const;
	virtual void			requestDisconnection(ClientId id, const std::string &reason, bool afterFlush);
	virtual enqueueResult	enqueueLine(ClientId id, const std::string &line);

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
	const std::vector<struct pollfd>	&getPollFds() const;
	const std::vector<int>				&getClientFds() const;
	ClientId							getLastAcceptedClientId() const;
	ClientId							getClientIdByFd(int fd) const;
	const netClientRecord				*getClientById(ClientId id) const;
	netClientRecord						*getClientById(ClientId id);
	const netClientRecord				*getClientByFd(int fd) const;
	protocol							*getProtocol() const;
	ChannelService						*getChannelService() const;
	std::string							getClientDisconnectReason(ClientId id) const;
	size_t								getLineBudget() const;
	size_t								getDispatchByteBudget() const;
	size_t								getReadByteBudget() const;
	size_t								getWriteByteBudget() const;
	size_t								getInBufHighWater() const;
	PollStatus							getLastPollStatus() const;
	int									getEffectivePollTimeout(int default_timout_ms) const;
	size_t								getClientPendingOutputSize(ClientId id) const;
	size_t								getClientOutputOffset(ClientId id) const;
	std::time_t							getFlushTimeoutSec() const;
	bool								isListening() const;
	bool								isListenerReady() const;
	bool								isIdExhausted() const;
	bool								isClientReadable(ClientId id) const;
	bool								isClientWritable(ClientId id) const;
	bool								isFdReadable(int fd) const;
	bool								isClientMarkedForCleanup(ClientId id) const;
	bool								isClientReadInterestEnabled(ClientId id) const;
	bool								isClientWriteInterestEnabled(ClientId id) const;
	bool								isClientClosingAfterFlush(ClientId id) const;
	static bool							isStopRequested();
	bool								isShuttingDown() const;
	bool								hasClientPendingOutput(ClientId id) const;
	bool								hasClientCompleteLine(ClientId id) const;
	bool								hasPendingCompleteLines() const;
	bool								hasClosingClients() const;
	
	//	Seter
	static void	ignoreSigpipe();
	static bool	installShutdownSignals();
	static void	resetStopFlag();
	void		setFlushTimeoutSec(std::time_t timeout_sec);
	size_t		enforceFlushCloseBounds();
	size_t		enforceFlushCloseBounds(std::time_t now);
	void		setProtocol(protocol *proto);
	void		createListenerSocket(int domain = AF_INET);
	void		configureListenerSocket();
	void		bindListenerSocket(in_addr_t bind_addr = INADDR_ANY);
	void		startListening(int backlog = LISTEN_BACKLOG);
	void		initListener(in_addr_t bind = INADDR_ANY, int Back_log = LISTEN_BACKLOG);
	void		closeListener();
	void		registerListenerInPoll_();
	int			waitForEvents(int timeout_ms = POLL_TIMEOUT_MS);
	int			handlePollResult(int poll_ret);
	int			injectPollResult(int simulated_ret);
	int			acceptReadyClient();
	bool		removeClientById(irc::ClientId id);
	bool		removeClientByFd(int fd);
	void		setNextClientId(irc::ClientId next_id);
	ssize_t		readReadyClient(ClientId id);
	ssize_t		readReadyClientByFd(int fd);
	size_t		readAllReadyClients();
	void		markClientForCleanup(ClientId id, const std::string &reason); 
	size_t		markErroredClientsForCleanup();
	ssize_t		handleRecvResult(ClientId id, const char *buf, ssize_t n);
	ssize_t		injectRecvResult(ClientId id, ssize_t simulated_ret);
	ssize_t		handleSendResult(ClientId id, ssize_t n);
	ssize_t		injectSendResult(ClientId id, ssize_t simulated_red);
	size_t		cleanupMarkedClients();
	void		setLineBudget(size_t budget);
	void		setDispatchByteBudget(size_t budget);
	void		setReadByteBudget(size_t budget);
	void		setWriteByteBudget(size_t budget);
	void		setInBufHighWater(size_t bytes);
	bool		extractNextClientLine(ClientId id, std::string &out_line);
	ssize_t		writeReadyClient(ClientId id);
	ssize_t		writeReadyClientByFd(int fd);
	size_t		writeAllReadyClients();
	size_t		dispatchClientLines(ClientId id);
	size_t		dispatchAllReadyLines();
	void		setChannelService(ChannelService *channel_service);
	bool		disconnectClient(ClientId id, const std::string &reason = "");
	void		shutdown(const std::string &reason = "Server shutdown");

	//	Debug
	std::string	getPSW() const;		//	Debug Use Only (Display raw _psw !!!)

	//	PasswordSource (PROTO-15)
	const std::string	&getPassword() const;
};

std::ostream	&operator<<(std::ostream &os, const ServerConfig &src);

}	//	namespace irc

#endif	// SERVERCONFIG_HPP
