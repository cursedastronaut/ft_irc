#include "Test.hpp"
#include "contracts/ChannelService.hpp"
#include "contracts/Protocol.hpp"
#include "net/ScopedSocket.hpp"
#include "net/ServerConfig.hpp"

#include <cstring>
#include <map>
#include <poll.h>
#include <set>
#include <string>
#include <sys/socket.h>
#include <vector>

namespace
{

class RecordingChannelStub : public irc::ChannelService
{
public:
	irc::ServerConfig			*server;
	std::vector<std::string>	*events;
	std::set<irc::ClientId>		members;
	std::map<irc::ClientId, int>	expected_fds;
	bool						trigger_reentrant;
	bool						saw_transport_alive;
	std::size_t					remove_calls;

	explicit RecordingChannelStub(std::vector<std::string> *log)
		: server(0), events(log), members(), expected_fds(),
		  trigger_reentrant(false), saw_transport_alive(false), remove_calls(0)
	{
	}

	virtual void handle(irc::ClientId, const irc::ParsedCommand &) {}

	virtual void removeClient(irc::ClientId actorId)
	{
		++remove_calls;
		events->push_back("channel.removeClient");
		std::map<irc::ClientId, int>::const_iterator it = expected_fds.find(actorId);
		if (server != 0 && it != expected_fds.end())
		{
			saw_transport_alive = (server->getClientById(actorId) != 0
				&& server->getClientIdByFd(it->second) == actorId);
		}
		if (trigger_reentrant && server != 0)
		{
			CHECK(!server->disconnectClient(actorId, "reentrant-from-channel"));
			CHECK(!server->removeClientById(actorId));
		}
		members.erase(actorId);
	}

	virtual std::vector<irc::ClientId> peersOf(irc::ClientId) const
	{
		return std::vector<irc::ClientId>();
	}
};

class RecordingProtocolStub : public protocol
{
public:
	irc::ServerConfig					*server;
	RecordingChannelStub				*channels;
	std::vector<std::string>			*events;
	std::map<irc::ClientId, std::string>	identities;
	std::map<irc::ClientId, int>			expected_fds;
	std::string							last_reason;
	bool								trigger_reentrant;
	bool								saw_identity_alive;
	bool								saw_channel_alive;
	bool								saw_transport_alive;
	std::size_t							disconnect_calls;

	RecordingProtocolStub(RecordingChannelStub *ch, std::vector<std::string> *log)
		: server(0), channels(ch), events(log), identities(), expected_fds(),
		  last_reason(), trigger_reentrant(false), saw_identity_alive(false),
		  saw_channel_alive(false), saw_transport_alive(false), disconnect_calls(0)
	{
	}

	virtual void connected(irc::ClientId id)
	{
		identities[id] = "live_nick";
	}

	virtual void online(irc::ClientId, const std::string &) {}

	virtual void disconnected(irc::ClientId id, const std::string &reason)
	{
		++disconnect_calls;
		last_reason = reason;
		events->push_back("protocol.disconnected");

		saw_identity_alive = (identities.find(id) != identities.end());
		saw_channel_alive = (channels != 0
			&& channels->members.find(id) != channels->members.end());
		std::map<irc::ClientId, int>::const_iterator it = expected_fds.find(id);
		if (server != 0 && it != expected_fds.end())
		{
			saw_transport_alive = (server->getClientById(id) != 0
				&& server->getClientIdByFd(it->second) == id);
		}
		if (trigger_reentrant && server != 0)
		{
			server->markClientForCleanup(id, "reentrant-mark");
			CHECK(!server->disconnectClient(id, "reentrant-from-protocol"));
			CHECK(!server->removeClientById(id));
			CHECK(server->cleanupMarkedClients() == 0);
		}
		identities.erase(id);
	}
};

int connectToListener(irc::ServerConfig &server, irc::ScopedSocket &peer_sock,
					  irc::ClientId &out_id)
{
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	std::memset(&addr, 0, sizeof(addr));
	CHECK(::getsockname(server.getListenFd(),
						reinterpret_cast<struct sockaddr *>(&addr), &len) == 0);

	peer_sock.openTcp(AF_INET);
	CHECK(::connect(peer_sock.get(),
					reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) == 0);
	CHECK(server.waitForEvents(500) > 0);
	const int accepted_fd = server.acceptReadyClient();
	CHECK(accepted_fd >= 0);
	out_id = server.getLastAcceptedClientId();
	CHECK(out_id != 0);
	return accepted_fd;
}

bool peerObservesEof(int peer_fd)
{
	struct pollfd pfd;
	pfd.fd = peer_fd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	if (::poll(&pfd, 1, 500) <= 0)
		return false;
	char byte = 0;
	return ::recv(peer_fd, &byte, 1, 0) == 0;
}

static void disconnectFollowsApprovedOrderAndPreservesOtherClients()
{
	std::vector<std::string> events;
	RecordingChannelStub channels(&events);
	RecordingProtocolStub proto(&channels, &events);
	irc::ServerConfig server(0, "net27-fixture-secret", &proto, &channels);
	channels.server = &server;
	proto.server = &server;
	server.initListener(INADDR_LOOPBACK, LISTEN_BACKLOG);

	irc::ScopedSocket peer1;
	irc::ScopedSocket peer2;
	irc::ClientId id1 = 0;
	irc::ClientId id2 = 0;
	const int fd1 = connectToListener(server, peer1, id1);
	const int fd2 = connectToListener(server, peer2, id2);

	channels.members.insert(id1);
	channels.members.insert(id2);
	channels.expected_fds[id1] = fd1;
	proto.expected_fds[id1] = fd1;

	CHECK(server.handleRecvResult(id1, "PARTIAL", 7) == 7);
	server.markClientForCleanup(id1, "Peer closed connection");
	server.markClientForCleanup(id1, "Duplicate reason ignored");
	CHECK(server.getClientDisconnectReason(id1) == "Peer closed connection");

	CHECK(server.cleanupMarkedClients() == 1);

	// 1. Exact approved ordering: protocol -> channel -> transport release/close.
	CHECK(events.size() == 2);
	CHECK(events[0] == "protocol.disconnected");
	CHECK(events[1] == "channel.removeClient");
	CHECK(proto.last_reason == "Peer closed connection");
	CHECK(proto.saw_identity_alive);
	CHECK(proto.saw_channel_alive);
	CHECK(proto.saw_transport_alive);
	CHECK(channels.saw_transport_alive);

	// 2. All state for id1 is released and its descriptor is closed.
	CHECK(proto.identities.find(id1) == proto.identities.end());
	CHECK(channels.members.find(id1) == channels.members.end());
	CHECK(server.getClientById(id1) == 0);
	CHECK(server.getClientByFd(fd1) == 0);
	CHECK(server.getClientIdByFd(fd1) == 0);
	CHECK(server.getClientNb() == 1);
	CHECK(peerObservesEof(peer1.get()));

	// 3. Unrelated client id2 remains untouched.
	CHECK(server.getClientById(id2) != 0);
	CHECK(server.getClientIdByFd(fd2) == id2);
	CHECK(proto.identities.find(id2) != proto.identities.end());
	CHECK(channels.members.find(id2) != channels.members.end());
}

static void repeatedAndUnknownDisconnectsAreIdempotent()
{
	std::vector<std::string> events;
	RecordingChannelStub channels(&events);
	RecordingProtocolStub proto(&channels, &events);
	irc::ServerConfig server(0, "net27-fixture-secret", &proto, &channels);
	channels.server = &server;
	proto.server = &server;
	server.initListener(INADDR_LOOPBACK, LISTEN_BACKLOG);

	irc::ScopedSocket peer;
	irc::ClientId id = 0;
	const int fd = connectToListener(server, peer, id);
	channels.members.insert(id);
	channels.expected_fds[id] = fd;
	proto.expected_fds[id] = fd;

	CHECK(server.disconnectClient(id, "Quit: leaving"));
	CHECK(!server.disconnectClient(id, "Quit: duplicate"));
	CHECK(!server.removeClientById(id));
	CHECK(!server.removeClientByFd(fd));
	CHECK(server.cleanupMarkedClients() == 0);
	CHECK(!server.disconnectClient(999999UL, "Unknown client"));
	CHECK(!server.removeClientById(999999UL));

	CHECK(proto.disconnect_calls == 1);
	CHECK(channels.remove_calls == 1);
	CHECK(events.size() == 2);
	CHECK(peerObservesEof(peer.get()));
}

static void reentrantDisconnectInsideHooksIsIdempotent()
{
	std::vector<std::string> events;
	RecordingChannelStub channels(&events);
	RecordingProtocolStub proto(&channels, &events);
	proto.trigger_reentrant = true;
	channels.trigger_reentrant = true;

	irc::ServerConfig server(0, "net27-fixture-secret", &proto, &channels);
	channels.server = &server;
	proto.server = &server;
	server.initListener(INADDR_LOOPBACK, LISTEN_BACKLOG);

	irc::ScopedSocket peer;
	irc::ClientId id = 0;
	const int fd = connectToListener(server, peer, id);
	channels.members.insert(id);
	channels.expected_fds[id] = fd;
	proto.expected_fds[id] = fd;

	CHECK(server.disconnectClient(id, "Reentrant disconnect"));
	CHECK(events.size() == 2);
	CHECK(events[0] == "protocol.disconnected");
	CHECK(events[1] == "channel.removeClient");
	CHECK(proto.disconnect_calls == 1);
	CHECK(channels.remove_calls == 1);
	CHECK(server.getClientById(id) == 0);
	CHECK(server.getClientNb() == 0);
	CHECK(peerObservesEof(peer.get()));
}

static void disconnectOrderTests()
{
	disconnectFollowsApprovedOrderAndPreservesOtherClients();
	repeatedAndUnknownDisconnectsAreIdempotent();
	reentrantDisconnectInsideHooksIsIdempotent();
}

} // namespace

int main()
{
	return runTests("NET-27 cross-subsystem disconnect order", disconnectOrderTests);
}
