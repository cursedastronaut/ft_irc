#include "net/ServerConfig.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

namespace
{

int	g_failures = 0;

#define CHECK(cond) \
	do { \
		if (!(cond)) { \
			std::cerr << "FAIL [" << __FILE__ << ":" << __LINE__ << "]: " << #cond << std::endl; \
			++g_failures; \
		} \
	} while (0)

bool	isFdClosed(int fd) {
	return (fd >= 0 && ::fcntl(fd, F_GETFD) == -1);
}

int	getBoundPort(int listen_fd) {
	struct sockaddr_in	addr;
	socklen_t			len = sizeof(addr);
	std::memset(&addr, 0, sizeof(addr));
	if (::getsockname(listen_fd, reinterpret_cast<struct sockaddr *>(&addr), &len) < 0)
		return (-1);
	return (static_cast<int>(ntohs(addr.sin_port)));
}

int	connectLoopback(int port) {
	int	fd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		return (-1);
	struct sockaddr_in	addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(static_cast<uint16_t>(port));
	if (::connect(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0) {
		::close(fd);
		return (-1);
	}
	return (fd);
}

struct LifecycleLog {
	std::vector<std::string>	events;
	std::vector<irc::ClientId>	disconnected_ids;
	std::vector<std::string>	disconnected_reasons;
	std::vector<irc::ClientId>	removed_channel_ids;
	size_t						online_calls;

	LifecycleLog() : online_calls(0) {}
};

class SpyProtocol : public protocol {
	private:
		LifecycleLog		&_log;
		irc::ServerConfig	*_server;
		bool				_attempt_reentry;
		irc::ClientId		_peer_target;

	public:
		SpyProtocol(LifecycleLog &log, irc::ServerConfig *server = NULL,
				bool attempt_reentry = false)
			: _log(log), _server(server), _attempt_reentry(attempt_reentry),
				_peer_target(0) {}

		void	setPeerTarget(irc::ClientId id) {
			_peer_target = id;
		}

		virtual void	connected(irc::ClientId id) {
			(void)id;
			_log.events.push_back("connected");
		}

		virtual void	online(irc::ClientId id, const std::string &line) {
			(void)id;
			(void)line;
			++_log.online_calls;
			_log.events.push_back("online");
		}

		virtual void	disconnected(irc::ClientId id, const std::string &reason) {
			_log.disconnected_ids.push_back(id);
			_log.disconnected_reasons.push_back(reason);
			_log.events.push_back("protocol::disconnected");

			if (_attempt_reentry && _server != NULL) {
				CHECK(_server->isShuttingDown());
				CHECK(!_server->isListening());
				CHECK(_server->getListenFd() == -1);
				CHECK(_server->acceptReadyClient() == -1);
				CHECK(_server->dispatchAllReadyLines() == 0);
				if (_peer_target != 0) {
					CHECK(_server->dispatchClientLines(_peer_target) == 0);
					CHECK(_server->enqueueLine(_peer_target, "NOTICE :blocked") == Rejected);
					_server->requestDisconnection(_peer_target, "reentrant", true);
					CHECK(!_server->isClientClosingAfterFlush(_peer_target));
				}
				CHECK(_server->readAllReadyClients() == 0);
				CHECK(_server->writeAllReadyClients() == 0);
				_server->shutdown("Recursive shutdown");
			}
		}
};

class SpyChannelService : public irc::ChannelService {
	private:
		LifecycleLog	&_log;

	public:
		explicit SpyChannelService(LifecycleLog &log) : _log(log) {}

		virtual void	handle(irc::ClientId actorId, const irc::ParsedCommand &command) {
			(void)actorId;
			(void)command;
		}

		virtual void	removeClient(irc::ClientId actorId) {
			_log.removed_channel_ids.push_back(actorId);
			_log.events.push_back("channel::removeClient");
		}

		virtual std::vector<irc::ClientId>	peersOf(irc::ClientId actorId) const {
			(void)actorId;
			return (std::vector<irc::ClientId>());
		}
};

bool	acceptClientOnServer(irc::ServerConfig &serv, int port,
			int &peer_fd_out, int &server_fd_out, irc::ClientId &id_out) {
	peer_fd_out = connectLoopback(port);
	if (peer_fd_out < 0)
		return (false);
	int	ready = serv.waitForEvents(500);
	if (ready <= 0 || !serv.isListenerReady()) {
		::close(peer_fd_out);
		peer_fd_out = -1;
		return (false);
	}
	server_fd_out = serv.acceptReadyClient();
	if (server_fd_out < 0) {
		::close(peer_fd_out);
		peer_fd_out = -1;
		return (false);
	}
	id_out = serv.getLastAcceptedClientId();
	return (true);
}

void	testShutdownWithSeveralClients() {
	LifecycleLog		log;
	SpyProtocol			proto(log);
	SpyChannelService	channels(log);
	irc::ServerConfig	serv(0, "test_pass", &proto, &channels);

	serv.initListener(INADDR_LOOPBACK, 8);
	int	listen_fd = serv.getListenFd();
	int	port = getBoundPort(listen_fd);
	CHECK(listen_fd >= 0);
	CHECK(port > 0);

	int				peer1 = -1, sfd1 = -1;
	int				peer2 = -1, sfd2 = -1;
	int				peer3 = -1, sfd3 = -1;
	irc::ClientId	id1 = 0, id2 = 0, id3 = 0;

	CHECK(acceptClientOnServer(serv, port, peer1, sfd1, id1));
	CHECK(acceptClientOnServer(serv, port, peer2, sfd2, id2));
	CHECK(acceptClientOnServer(serv, port, peer3, sfd3, id3));
	CHECK(serv.getClientNb() == 3);

	// Client 1: ligne complete en buffer qui ne doit PAS etre dispatchee au shutdown
	const char	*msg = "PING :should_not_dispatch\r\n";
	CHECK(::send(peer1, msg, std::strlen(msg), 0) > 0);
	CHECK(serv.waitForEvents(500) > 0);
	CHECK(serv.readAllReadyClients() == 1);
	CHECK(serv.hasClientCompleteLine(id1));

	// Client 2: sortie en attente
	CHECK(serv.enqueueLine(id2, "NOTICE :pending_output") == Queued);
	CHECK(serv.hasClientPendingOutput(id2));

	// Client 3: etat close_after_flush
	CHECK(serv.enqueueLine(id3, "ERROR :closing") == Queued);
	serv.requestDisconnection(id3, "Client quit", true);
	CHECK(serv.isClientClosingAfterFlush(id3));

	serv.shutdown("Server shutdown");

	// Verifie l'absence totale de descripteurs et d'etat possedes
	CHECK(!serv.isListening());
	CHECK(!serv.isShuttingDown());
	CHECK(serv.getListenFd() == -1);
	CHECK(serv.getClientNb() == 0);
	CHECK(serv.getPollFds().empty());
	CHECK(serv.getClientFds().empty());
	CHECK(serv.getLastAcceptedClientId() == 0);
	CHECK(!serv.hasPendingCompleteLines());
	CHECK(!serv.hasClosingClients());
	CHECK(serv.getClientById(id1) == NULL);
	CHECK(serv.getClientById(id2) == NULL);
	CHECK(serv.getClientById(id3) == NULL);
	CHECK(serv.getClientByFd(sfd1) == NULL);
	CHECK(serv.getClientByFd(sfd2) == NULL);
	CHECK(serv.getClientByFd(sfd3) == NULL);

	CHECK(isFdClosed(listen_fd));
	CHECK(isFdClosed(sfd1));
	CHECK(isFdClosed(sfd2));
	CHECK(isFdClosed(sfd3));

	// Verifie le cycle de vie partage et l'absence de dispatch
	CHECK(log.online_calls == 0);
	CHECK(log.disconnected_ids.size() == 3);
	CHECK(log.removed_channel_ids.size() == 3);
	CHECK(log.disconnected_reasons.size() == 3);
	if (log.disconnected_reasons.size() == 3) {
		CHECK(log.disconnected_reasons[0] == "Server shutdown");
		CHECK(log.disconnected_reasons[1] == "Server shutdown");
		CHECK(log.disconnected_reasons[2] == "Client quit");
	}

	// Idempotence d'un second shutdown
	serv.shutdown("Second shutdown");
	CHECK(log.disconnected_ids.size() == 3);
	CHECK(log.removed_channel_ids.size() == 3);
	CHECK(log.online_calls == 0);

	// L'admission est bien arretee
	int	late_peer = connectLoopback(port);
	CHECK(late_peer < 0);
	if (late_peer >= 0)
		::close(late_peer);

	::close(peer1);
	::close(peer2);
	::close(peer3);
}

void	testShutdownNoDispatchReentry() {
	LifecycleLog		log;
	irc::ServerConfig	serv(0, "test_pass");
	SpyProtocol			proto(log, &serv, true);
	SpyChannelService	channels(log);
	serv.setProtocol(&proto);
	serv.setChannelService(&channels);

	serv.initListener(INADDR_LOOPBACK, 8);
	int	port = getBoundPort(serv.getListenFd());
	CHECK(port > 0);

	int				peer1 = -1, sfd1 = -1;
	int				peer2 = -1, sfd2 = -1;
	irc::ClientId	id1 = 0, id2 = 0;

	CHECK(acceptClientOnServer(serv, port, peer1, sfd1, id1));
	CHECK(acceptClientOnServer(serv, port, peer2, sfd2, id2));
	proto.setPeerTarget(id2);

	const char	*line = "NICK alice\r\n";
	CHECK(::send(peer1, line, std::strlen(line), 0) > 0);
	CHECK(::send(peer2, line, std::strlen(line), 0) > 0);
	CHECK(serv.waitForEvents(500) > 0);
	CHECK(serv.readAllReadyClients() >= 1);
	if (!serv.hasClientCompleteLine(id2)) {
		CHECK(serv.waitForEvents(500) > 0);
		serv.readAllReadyClients();
	}
	CHECK(serv.hasClientCompleteLine(id1));
	CHECK(serv.hasClientCompleteLine(id2));

	serv.shutdown("Graceful stop");

	CHECK(log.online_calls == 0);
	CHECK(log.disconnected_ids.size() == 2);
	CHECK(log.removed_channel_ids.size() == 2);
	CHECK(isFdClosed(sfd1));
	CHECK(isFdClosed(sfd2));
	CHECK(serv.getClientNb() == 0);
	CHECK(serv.getPollFds().empty());

	::close(peer1);
	::close(peer2);
}

void	testShutdownAfterPartialInitialization() {
	// 1. Juste construit, aucun socket cree
	{
		irc::ServerConfig	serv(0, "test_pass");
		serv.shutdown();
		CHECK(serv.getListenFd() == -1);
		CHECK(!serv.isListening());
		CHECK(serv.getPollFds().empty());
	}

	// 2. Apres createListenerSocket() uniquement
	{
		irc::ServerConfig	serv(0, "test_pass");
		serv.createListenerSocket(AF_INET);
		int	fd = serv.getListenFd();
		CHECK(fd >= 0);
		serv.shutdown();
		CHECK(serv.getListenFd() == -1);
		CHECK(isFdClosed(fd));
	}

	// 3. Apres configureListenerSocket() uniquement
	{
		irc::ServerConfig	serv(0, "test_pass");
		serv.createListenerSocket(AF_INET);
		serv.configureListenerSocket();
		int	fd = serv.getListenFd();
		CHECK(fd >= 0);
		serv.shutdown();
		CHECK(serv.getListenFd() == -1);
		CHECK(isFdClosed(fd));
	}

	// 4. Apres bindListenerSocket() sans listen() ni poll
	{
		irc::ServerConfig	serv(0, "test_pass");
		serv.createListenerSocket(AF_INET);
		serv.configureListenerSocket();
		serv.bindListenerSocket(INADDR_LOOPBACK);
		int	fd = serv.getListenFd();
		CHECK(fd >= 0);
		serv.shutdown();
		CHECK(serv.getListenFd() == -1);
		CHECK(isFdClosed(fd));
	}

	// 5. Apres startListening() sans registerListenerInPoll_()
	{
		irc::ServerConfig	serv(0, "test_pass");
		serv.createListenerSocket(AF_INET);
		serv.configureListenerSocket();
		serv.bindListenerSocket(INADDR_LOOPBACK);
		serv.startListening(8);
		int	fd = serv.getListenFd();
		CHECK(fd >= 0);
		CHECK(serv.isListening());
		serv.shutdown();
		CHECK(!serv.isListening());
		CHECK(serv.getListenFd() == -1);
		CHECK(isFdClosed(fd));
	}

	// 6. Echec de bind (initialisation partielle echouee)
	{
		irc::ServerConfig	holder(0, "test_pass");
		holder.initListener(INADDR_LOOPBACK, 8);
		int	occupied_port = getBoundPort(holder.getListenFd());
		CHECK(occupied_port > 0);

		irc::ServerConfig	failing(occupied_port, "test_pass");
		bool				caught = false;
		try {
			failing.initListener(INADDR_LOOPBACK, 8);
		}
		catch (const irc::ServerConfig::SocketBindException &) {
			caught = true;
		}
		CHECK(caught);
		CHECK(failing.getListenFd() == -1);
		CHECK(!failing.isListening());
		failing.shutdown();
		holder.shutdown();
	}
}

void	testDestructorReleasesAllResources() {
	LifecycleLog		log;
	SpyProtocol			proto(log);
	SpyChannelService	channels(log);
	int					listen_fd = -1;
	int					sfd1 = -1;
	int					peer1 = -1;

	{
		irc::ServerConfig	serv(0, "test_pass", &proto, &channels);
		serv.initListener(INADDR_LOOPBACK, 8);
		listen_fd = serv.getListenFd();
		int				port = getBoundPort(listen_fd);
		irc::ClientId	id1 = 0;
		CHECK(acceptClientOnServer(serv, port, peer1, sfd1, id1));
	}

	CHECK(isFdClosed(listen_fd));
	CHECK(isFdClosed(sfd1));
	CHECK(log.disconnected_ids.size() == 1);
	CHECK(log.removed_channel_ids.size() == 1);
	CHECK(log.online_calls == 0);

	if (peer1 >= 0)
		::close(peer1);
}

}	// namespace

int	main() {
	testShutdownWithSeveralClients();
	testShutdownNoDispatchReentry();
	testShutdownAfterPartialInitialization();
	testDestructorReleasesAllResources();

	if (g_failures != 0) {
		std::cerr << "FAILED: " << g_failures << " check(s) failed." << std::endl;
		return (1);
	}
	std::cout << "PASS: test_shutdown" << std::endl;
	return (0);
}
