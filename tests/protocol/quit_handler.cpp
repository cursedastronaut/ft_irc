#include "Test.hpp"
#include "commands/auth/QuitHandler.hpp"
#include "protocol/ClientRegistry.hpp"

namespace
{

class FakeTransport : public transport
{
	public:
		ClientIdentity returnIdentity(irc::ClientId id) const
		{
			(void)id;
			return ClientIdentity();
		}

		void requestDisconnection(irc::ClientId id, const std::string &reason,
								   bool afterFlush)
		{
			disconnectRequested = true;
			disconnectedId = id;
			disconnectReason = reason;
			disconnectAfterFlush = afterFlush;
		}

		enqueueResult enqueueLine(irc::ClientId id, const std::string &line)
		{
			ids.push_back(id);
			lines.push_back(line);
			return Queued;
		}

		std::vector<irc::ClientId> ids;
		std::vector<std::string> lines;
		bool disconnectRequested;
		irc::ClientId disconnectedId;
		std::string disconnectReason;
		bool disconnectAfterFlush;

		FakeTransport() : disconnectRequested(false), disconnectedId(0),
						  disconnectAfterFlush(false) {}
};

class FakeChannelService : public irc::ChannelService
{
	public:
		FakeChannelService() : removeClientCallCount(0) {}

		void handle(irc::ClientId, const irc::ParsedCommand &) {}

		void removeClient(irc::ClientId actorId)
		{
			++removeClientCallCount;
			lastRemoved = actorId;
		}

		std::vector<irc::ClientId> peersOf(irc::ClientId actorId) const
		{
			(void)actorId;
			return configuredPeers;
		}

		std::vector<irc::ClientId> configuredPeers;
		int removeClientCallCount;
		irc::ClientId lastRemoved;
};

void registerClient(irc::ClientRegistry &registry, irc::ClientId id,
					 const std::string &nickname, const std::string &username)
{
	registry.create(id);
	registry.setNickname(id, nickname);
	registry.setUser(id, username, "Real Name");
	registry.setPasswordAccepted(id, true);
}

void unknownClientDoesNothing()
{
	irc::ClientRegistry registry;
	FakeTransport net;
	FakeChannelService channels;
	irc::QuitHandler quit(registry, net, channels);

	quit.handle(1, std::vector<std::string>());

	CHECK(net.lines.empty());
	CHECK(net.disconnectRequested == false);
}

void noReasonGivenDefaultsToClientQuit()
{
	irc::ClientRegistry registry;
	FakeTransport net;
	FakeChannelService channels;
	irc::QuitHandler quit(registry, net, channels);
	registerClient(registry, 1, "alice", "al");

	quit.handle(1, std::vector<std::string>());

	CHECK(net.disconnectRequested == true);
	CHECK(net.disconnectedId == 1);
	CHECK(net.disconnectReason == "Client Quit");
	CHECK(net.disconnectAfterFlush == true);
}

void explicitReasonIsUsed()
{
	irc::ClientRegistry registry;
	FakeTransport net;
	FakeChannelService channels;
	irc::QuitHandler quit(registry, net, channels);
	registerClient(registry, 1, "alice", "al");

	std::vector<std::string> params;
	params.push_back("au revoir tout le monde");
	quit.handle(1, params);

	CHECK(net.disconnectReason == "au revoir tout le monde");
}

void notifiesEachPeerExactlyOnceWithTheQuitLine()
{
	irc::ClientRegistry registry;
	FakeTransport net;
	FakeChannelService channels;
	irc::QuitHandler quit(registry, net, channels);
	registerClient(registry, 1, "alice", "al");
	channels.configuredPeers.push_back(2);
	channels.configuredPeers.push_back(3);

	std::vector<std::string> params;
	params.push_back("bye");
	quit.handle(1, params);

	CHECK(net.lines.size() == 2);
	CHECK(net.ids[0] == 2);
	CHECK(net.ids[1] == 3);
	CHECK(net.lines[0] == ":alice!al@ircserv QUIT :bye");
	CHECK(net.lines[1] == ":alice!al@ircserv QUIT :bye");
	CHECK(net.disconnectRequested == true);
	CHECK(net.disconnectedId == 1);
}

void noPeersStillRequestsDisconnection()
{
	irc::ClientRegistry registry;
	FakeTransport net;
	FakeChannelService channels;
	irc::QuitHandler quit(registry, net, channels);
	registerClient(registry, 1, "alice", "al");

	quit.handle(1, std::vector<std::string>());

	CHECK(net.lines.empty());
	CHECK(net.disconnectRequested == true);
}

void unregisteredClientStillRequestsDisconnectionWithoutNotifying()
{
	irc::ClientRegistry registry;
	FakeTransport net;
	FakeChannelService channels;
	irc::QuitHandler quit(registry, net, channels);
	registry.create(1);
	// no nickname/user/password: never registered, no channel can exist for it

	quit.handle(1, std::vector<std::string>());

	CHECK(net.lines.empty());
	CHECK(net.disconnectRequested == true);
	CHECK(net.disconnectedId == 1);
}

void quitDoesNotRemoveFromRegistryOrChannelsItself()
{
	irc::ClientRegistry registry;
	FakeTransport net;
	FakeChannelService channels;
	irc::QuitHandler quit(registry, net, channels);
	registerClient(registry, 1, "alice", "al");

	quit.handle(1, std::vector<std::string>());

	irc::ClientRecord stillThere;
	CHECK(registry.find(1, stillThere) == true);
	CHECK(channels.removeClientCallCount == 0);
}

}

int main()
{
	int failures = 0;

	failures += runTests("unknownClientDoesNothing", unknownClientDoesNothing);
	failures += runTests("noReasonGivenDefaultsToClientQuit",
						  noReasonGivenDefaultsToClientQuit);
	failures += runTests("explicitReasonIsUsed", explicitReasonIsUsed);
	failures += runTests("notifiesEachPeerExactlyOnceWithTheQuitLine",
						  notifiesEachPeerExactlyOnceWithTheQuitLine);
	failures += runTests("noPeersStillRequestsDisconnection",
						  noPeersStillRequestsDisconnection);
	failures += runTests("unregisteredClientStillRequestsDisconnectionWithoutNotifying",
						  unregisteredClientStillRequestsDisconnectionWithoutNotifying);
	failures += runTests("quitDoesNotRemoveFromRegistryOrChannelsItself",
						  quitDoesNotRemoveFromRegistryOrChannelsItself);

	return failures;
}
