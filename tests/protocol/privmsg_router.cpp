#include "Test.hpp"
#include "commands/auth/PrivmsgRouter.hpp"
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
			(void)id;
			(void)reason;
			(void)afterFlush;
		}

		enqueueResult enqueueLine(irc::ClientId id, const std::string &line)
		{
			ids.push_back(id);
			lines.push_back(line);
			return Queued;
		}

		std::vector<irc::ClientId> ids;
		std::vector<std::string> lines;
};

class SpyChannelHandler : public irc::CommandHandler
{
	public:
		SpyChannelHandler() : callCount(0) {}

		void handle(irc::ClientId actor, const std::vector<std::string> &params)
		{
			++callCount;
			lastActor = actor;
			lastParams = params;
		}

		int callCount;
		irc::ClientId lastActor;
		std::vector<std::string> lastParams;
};

void registerClient(irc::ClientRegistry &registry, irc::ClientId id,
					 const std::string &nickname, const std::string &username)
{
	registry.create(id);
	registry.setNickname(id, nickname);
	registry.setUser(id, username, "Real Name");
	registry.setPasswordAccepted(id, true);
}

void missingRecipientReplies411AndDoesNotDeliverOrDelegate()
{
	irc::ClientRegistry registry;
	FakeTransport net;
	SpyChannelHandler channel;
	irc::PrivmsgRouter router(registry, net, channel);
	registerClient(registry, 1, "alice", "al");

	router.handle(1, std::vector<std::string>());

	CHECK(net.lines.size() == 1);
	CHECK(net.ids[0] == 1);
	CHECK(net.lines[0] == ":ircserv 411 alice :No recipient given (PRIVMSG)");
	CHECK(channel.callCount == 0);
}

void emptyRecipientStringReplies411()
{
	irc::ClientRegistry registry;
	FakeTransport net;
	SpyChannelHandler channel;
	irc::PrivmsgRouter router(registry, net, channel);
	registerClient(registry, 1, "alice", "al");

	std::vector<std::string> params;
	params.push_back("");
	params.push_back("hello");
	router.handle(1, params);

	CHECK(net.lines.size() == 1);
	CHECK(net.lines[0] == ":ircserv 411 alice :No recipient given (PRIVMSG)");
	CHECK(channel.callCount == 0);
}

void privateTargetWithMissingTextReplies412()
{
	irc::ClientRegistry registry;
	FakeTransport net;
	SpyChannelHandler channel;
	irc::PrivmsgRouter router(registry, net, channel);
	registerClient(registry, 1, "alice", "al");

	std::vector<std::string> params;
	params.push_back("bob");
	router.handle(1, params);

	CHECK(net.lines.size() == 1);
	CHECK(net.lines[0] == ":ircserv 412 alice :No text to send");
	CHECK(channel.callCount == 0);
}

void privateTargetWithEmptyTextReplies412()
{
	irc::ClientRegistry registry;
	FakeTransport net;
	SpyChannelHandler channel;
	irc::PrivmsgRouter router(registry, net, channel);
	registerClient(registry, 1, "alice", "al");

	std::vector<std::string> params;
	params.push_back("bob");
	params.push_back("");
	router.handle(1, params);

	CHECK(net.lines.size() == 1);
	CHECK(net.lines[0] == ":ircserv 412 alice :No text to send");
	CHECK(channel.callCount == 0);
}

void privateTargetDeliversExactTrailingTextOnceToTheRecipientOnly()
{
	irc::ClientRegistry registry;
	FakeTransport net;
	SpyChannelHandler channel;
	irc::PrivmsgRouter router(registry, net, channel);
	registerClient(registry, 1, "alice", "al");
	registerClient(registry, 2, "bob", "bo");

	std::vector<std::string> params;
	params.push_back("bob");
	params.push_back("hello there");
	router.handle(1, params);

	CHECK(net.lines.size() == 1);
	CHECK(net.ids[0] == 2);
	CHECK(net.lines[0] == ":alice!al@ircserv PRIVMSG bob :hello there");
	CHECK(channel.callCount == 0);
}

void unknownNicknameTargetReplies401AndDoesNotDeliver()
{
	irc::ClientRegistry registry;
	FakeTransport net;
	SpyChannelHandler channel;
	irc::PrivmsgRouter router(registry, net, channel);
	registerClient(registry, 1, "alice", "al");

	std::vector<std::string> params;
	params.push_back("ghost");
	params.push_back("hi");
	router.handle(1, params);

	CHECK(net.lines.size() == 1);
	CHECK(net.ids[0] == 1);
	CHECK(net.lines[0] == ":ircserv 401 alice ghost :No such nick/channel");
	CHECK(channel.callCount == 0);
}

void targetThatDisconnectedJustBeforeDispatchBehavesLikeUnknownNickname()
{
	irc::ClientRegistry registry;
	FakeTransport net;
	SpyChannelHandler channel;
	irc::PrivmsgRouter router(registry, net, channel);
	registerClient(registry, 1, "alice", "al");
	registerClient(registry, 2, "bob", "bo");
	registry.remove(2);

	std::vector<std::string> params;
	params.push_back("bob");
	params.push_back("hi");
	router.handle(1, params);

	CHECK(net.lines.size() == 1);
	CHECK(net.ids[0] == 1);
	CHECK(net.lines[0] == ":ircserv 401 alice bob :No such nick/channel");
	CHECK(channel.callCount == 0);
}

void channelTargetDelegatesEntirelyToTheChannelHandler()
{
	irc::ClientRegistry registry;
	FakeTransport net;
	SpyChannelHandler channel;
	irc::PrivmsgRouter router(registry, net, channel);
	registerClient(registry, 1, "alice", "al");

	std::vector<std::string> params;
	params.push_back("#general");
	params.push_back("hi everyone");
	router.handle(1, params);

	CHECK(channel.callCount == 1);
	CHECK(channel.lastActor == 1);
	CHECK(channel.lastParams.size() == 2);
	CHECK(channel.lastParams[0] == "#general");
	CHECK(channel.lastParams[1] == "hi everyone");
	CHECK(net.lines.empty());
}

void ampersandPrefixedTargetAlsoDelegatesToTheChannelHandler()
{
	irc::ClientRegistry registry;
	FakeTransport net;
	SpyChannelHandler channel;
	irc::PrivmsgRouter router(registry, net, channel);
	registerClient(registry, 1, "alice", "al");

	std::vector<std::string> params;
	params.push_back("&local");
	params.push_back("hi");
	router.handle(1, params);

	CHECK(channel.callCount == 1);
	CHECK(net.lines.empty());
}

}

int main()
{
	int failures = 0;

	failures += runTests("missingRecipientReplies411AndDoesNotDeliverOrDelegate",
						  missingRecipientReplies411AndDoesNotDeliverOrDelegate);
	failures += runTests("emptyRecipientStringReplies411",
						  emptyRecipientStringReplies411);
	failures += runTests("privateTargetWithMissingTextReplies412",
						  privateTargetWithMissingTextReplies412);
	failures += runTests("privateTargetWithEmptyTextReplies412",
						  privateTargetWithEmptyTextReplies412);
	failures += runTests("privateTargetDeliversExactTrailingTextOnceToTheRecipientOnly",
						  privateTargetDeliversExactTrailingTextOnceToTheRecipientOnly);
	failures += runTests("unknownNicknameTargetReplies401AndDoesNotDeliver",
						  unknownNicknameTargetReplies401AndDoesNotDeliver);
	failures += runTests("targetThatDisconnectedJustBeforeDispatchBehavesLikeUnknownNickname",
						  targetThatDisconnectedJustBeforeDispatchBehavesLikeUnknownNickname);
	failures += runTests("channelTargetDelegatesEntirelyToTheChannelHandler",
						  channelTargetDelegatesEntirelyToTheChannelHandler);
	failures += runTests("ampersandPrefixedTargetAlsoDelegatesToTheChannelHandler",
						  ampersandPrefixedTargetAlsoDelegatesToTheChannelHandler);

	return failures;
}
