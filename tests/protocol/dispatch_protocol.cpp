#include "Test.hpp"
#include "protocol/DispatchProtocol.hpp"

namespace
{

class StubHandler : public irc::CommandHandler
{
	public:
		StubHandler() : callCount(0), lastActor(0) {}

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

class RecordingTransport : public transport
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

void connectedCreatesARegistryEntry()
{
	irc::Dispatcher dispatcher;
	irc::ClientRegistry registry;
	RecordingTransport net;
	irc::DispatchProtocol protocol(dispatcher, registry, net);
	irc::ClientRecord record;

	CHECK(registry.find(1, record) == false);
	protocol.connected(1);
	CHECK(registry.find(1, record) == true);
}

void disconnectedRemovesTheRegistryEntry()
{
	irc::Dispatcher dispatcher;
	irc::ClientRegistry registry;
	RecordingTransport net;
	irc::DispatchProtocol protocol(dispatcher, registry, net);
	irc::ClientRecord record;

	protocol.connected(1);
	CHECK(registry.find(1, record) == true);
	protocol.disconnected(1, "bye");
	CHECK(registry.find(1, record) == false);
}

void aRegisteredCommandReachesItsHandlerWithCorrectActorAndParams()
{
	irc::Dispatcher dispatcher;
	irc::ClientRegistry registry;
	StubHandler nick;
	dispatcher.registerHandler("NICK", &nick);
	RecordingTransport net;
	irc::DispatchProtocol protocol(dispatcher, registry, net);

	protocol.connected(42);
	protocol.online(42, "NICK bob");

	CHECK(nick.callCount == 1);
	CHECK(nick.lastActor == 42UL);
	CHECK(nick.lastParams.size() == 1U);
	CHECK(nick.lastParams[0] == "bob");
}

void anUnregisteredConnectedClientCannotReachARegularHandler()
{
	irc::Dispatcher dispatcher;
	irc::ClientRegistry registry;
	StubHandler privmsg;
	dispatcher.registerHandler("PRIVMSG", &privmsg);
	RecordingTransport net;
	irc::DispatchProtocol protocol(dispatcher, registry, net);

	protocol.connected(1);
	protocol.online(1, "PRIVMSG bob :hi");

	CHECK(privmsg.callCount == 0);
}

void anUnregisteredConnectedClientCanStillReachNick()
{
	irc::Dispatcher dispatcher;
	irc::ClientRegistry registry;
	StubHandler nick;
	dispatcher.registerHandler("NICK", &nick);
	RecordingTransport net;
	irc::DispatchProtocol protocol(dispatcher, registry, net);

	protocol.connected(1);
	protocol.online(1, "NICK bob");

	CHECK(nick.callCount == 1);
}

void anUnknownClientNeverConnectedCanStillReachPass()
{
	irc::Dispatcher dispatcher;
	irc::ClientRegistry registry;
	StubHandler pass;
	dispatcher.registerHandler("PASS", &pass);
	RecordingTransport net;
	irc::DispatchProtocol protocol(dispatcher, registry, net);

	protocol.online(1, "PASS secret");

	CHECK(pass.callCount == 1);
}

void anUnknownClientNeverConnectedCannotReachARegularHandler()
{
	irc::Dispatcher dispatcher;
	irc::ClientRegistry registry;
	StubHandler privmsg;
	dispatcher.registerHandler("PRIVMSG", &privmsg);
	RecordingTransport net;
	irc::DispatchProtocol protocol(dispatcher, registry, net);

	protocol.online(1, "PRIVMSG bob :hi");

	CHECK(privmsg.callCount == 0);
}

void onlineOnAnEmptyLineIsIgnoredSilently()
{
	irc::Dispatcher dispatcher;
	irc::ClientRegistry registry;
	StubHandler nick;
	dispatcher.registerHandler("NICK", &nick);
	RecordingTransport net;
	irc::DispatchProtocol protocol(dispatcher, registry, net);

	protocol.connected(1);
	protocol.online(1, "");

	CHECK(nick.callCount == 0);
}

void onlineOnAMalformedPrefixIsIgnoredSilently()
{
	irc::Dispatcher dispatcher;
	irc::ClientRegistry registry;
	StubHandler nick;
	dispatcher.registerHandler("NICK", &nick);
	RecordingTransport net;
	irc::DispatchProtocol protocol(dispatcher, registry, net);

	protocol.connected(1);
	protocol.online(1, ":");

	CHECK(nick.callCount == 0);
}

void aPreRegistrationCommandWithoutHandlerIsUnknown()
{
	irc::Dispatcher dispatcher;
	irc::ClientRegistry registry;
	RecordingTransport net;
	irc::DispatchProtocol protocol(dispatcher, registry, net);

	protocol.connected(1);
	protocol.online(1, "NICK bob");

	CHECK(net.lines.size() == 1U);
	CHECK(net.lines[0] == ":ircserv 421 * NICK :Unknown command");
}

void anUnknownCommandProducesOneReplyAndTheNextCommandStillWorks()
{
	irc::Dispatcher dispatcher;
	irc::ClientRegistry registry;
	StubHandler nick;
	dispatcher.registerHandler("NICK", &nick, 1);
	RecordingTransport net;
	irc::DispatchProtocol protocol(dispatcher, registry, net);

	protocol.connected(1);
	registry.setNickname(1, "bob");
	protocol.online(1, "USER bob 0 * :Bob");

	CHECK(net.lines.size() == 1U);
	CHECK(net.ids[0] == 1UL);
	CHECK(net.lines[0] == ":ircserv 421 bob USER :Unknown command");

	protocol.online(1, "NICK alice");

	CHECK(nick.callCount == 1);
	CHECK(net.lines.size() == 1U);
}

void missingParametersReplyWithoutReachingTheHandler()
{
	irc::Dispatcher dispatcher;
	irc::ClientRegistry registry;
	StubHandler nick;
	dispatcher.registerHandler("NICK", &nick, 1);
	RecordingTransport net;
	irc::DispatchProtocol protocol(dispatcher, registry, net);

	protocol.connected(1);
	protocol.online(1, "NICK");

	CHECK(nick.callCount == 0);
	CHECK(net.lines.size() == 1U);
	CHECK(net.lines[0] == ":ircserv 461 * NICK :Not enough parameters");

	protocol.online(1, "NICK bob");

	CHECK(nick.callCount == 1);
	CHECK(net.lines.size() == 1U);
}

void aHandledCommandSendsNoDispatcherError()
{
	irc::Dispatcher dispatcher;
	irc::ClientRegistry registry;
	StubHandler pass;
	dispatcher.registerHandler("PASS", &pass, 1);
	RecordingTransport net;
	irc::DispatchProtocol protocol(dispatcher, registry, net);

	protocol.connected(1);
	protocol.online(1, "PASS secret");

	CHECK(pass.callCount == 1);
	CHECK(net.lines.empty());
}

void capLsRepliesWithAnEmptyCapabilityList()
{
	irc::Dispatcher dispatcher;
	irc::ClientRegistry registry;
	RecordingTransport net;
	irc::DispatchProtocol protocol(dispatcher, registry, net);

	protocol.connected(1);
	protocol.online(1, "CAP LS");

	CHECK(net.lines.size() == 1U);
	CHECK(net.lines[0] == ":ircserv CAP * LS :");
}

void capLsWithAVersionArgumentRepliesTheSameWay()
{
	irc::Dispatcher dispatcher;
	irc::ClientRegistry registry;
	RecordingTransport net;
	irc::DispatchProtocol protocol(dispatcher, registry, net);

	protocol.connected(1);
	protocol.online(1, "CAP LS 302");

	CHECK(net.lines.size() == 1U);
	CHECK(net.lines[0] == ":ircserv CAP * LS :");
}

void capReqForAnUnsupportedCapabilityIsNeverAcknowledged()
{
	irc::Dispatcher dispatcher;
	irc::ClientRegistry registry;
	RecordingTransport net;
	irc::DispatchProtocol protocol(dispatcher, registry, net);

	protocol.connected(1);
	protocol.online(1, "CAP REQ :sasl");

	CHECK(net.lines.empty());
}

void capEndProducesNoReply()
{
	irc::Dispatcher dispatcher;
	irc::ClientRegistry registry;
	RecordingTransport net;
	irc::DispatchProtocol protocol(dispatcher, registry, net);

	protocol.connected(1);
	protocol.online(1, "CAP END");

	CHECK(net.lines.empty());
}

void bareCapBeforeRegistrationIsSilentlyDropped()
{
	irc::Dispatcher dispatcher;
	irc::ClientRegistry registry;
	RecordingTransport net;
	irc::DispatchProtocol protocol(dispatcher, registry, net);

	protocol.connected(1);
	protocol.online(1, "CAP");

	CHECK(net.lines.empty());
}

void bareCapAfterRegistrationIsAnUnknownCommand()
{
	irc::Dispatcher dispatcher;
	irc::ClientRegistry registry;
	RecordingTransport net;
	irc::DispatchProtocol protocol(dispatcher, registry, net);

	protocol.connected(1);
	registry.setNickname(1, "bob");
	registry.setRegistered(1);
	protocol.online(1, "CAP");

	CHECK(net.lines.size() == 1U);
	CHECK(net.lines[0] == ":ircserv 421 bob CAP :Unknown command");
}

void pingWithoutAParameterReplies461()
{
	irc::Dispatcher dispatcher;
	irc::ClientRegistry registry;
	RecordingTransport net;
	irc::DispatchProtocol protocol(dispatcher, registry, net);

	protocol.connected(1);
	protocol.online(1, "PING");

	CHECK(net.lines.size() == 1U);
	CHECK(net.lines[0] == ":ircserv 461 * PING :Not enough parameters");
}

void pingWithAParameterRepliesWithTheExactEcho()
{
	irc::Dispatcher dispatcher;
	irc::ClientRegistry registry;
	RecordingTransport net;
	irc::DispatchProtocol protocol(dispatcher, registry, net);

	protocol.connected(1);
	protocol.online(1, "PING foo");

	CHECK(net.lines.size() == 1U);
	CHECK(net.lines[0] == ":ircserv PONG ircserv :foo");
}

}

int main()
{
	int failures = 0;

	failures += runTests("connectedCreatesARegistryEntry",
						  connectedCreatesARegistryEntry);
	failures += runTests("disconnectedRemovesTheRegistryEntry",
						  disconnectedRemovesTheRegistryEntry);
	failures += runTests("aRegisteredCommandReachesItsHandlerWithCorrectActorAndParams",
						  aRegisteredCommandReachesItsHandlerWithCorrectActorAndParams);
	failures += runTests("anUnregisteredConnectedClientCannotReachARegularHandler",
						  anUnregisteredConnectedClientCannotReachARegularHandler);
	failures += runTests("anUnregisteredConnectedClientCanStillReachNick",
						  anUnregisteredConnectedClientCanStillReachNick);
	failures += runTests("anUnknownClientNeverConnectedCanStillReachPass",
						  anUnknownClientNeverConnectedCanStillReachPass);
	failures += runTests("anUnknownClientNeverConnectedCannotReachARegularHandler",
						  anUnknownClientNeverConnectedCannotReachARegularHandler);
	failures += runTests("onlineOnAnEmptyLineIsIgnoredSilently",
						  onlineOnAnEmptyLineIsIgnoredSilently);
	failures += runTests("onlineOnAMalformedPrefixIsIgnoredSilently",
						  onlineOnAMalformedPrefixIsIgnoredSilently);
	failures += runTests("aPreRegistrationCommandWithoutHandlerIsUnknown",
						  aPreRegistrationCommandWithoutHandlerIsUnknown);
	failures += runTests("anUnknownCommandProducesOneReplyAndTheNextCommandStillWorks",
						  anUnknownCommandProducesOneReplyAndTheNextCommandStillWorks);
	failures += runTests("missingParametersReplyWithoutReachingTheHandler",
						  missingParametersReplyWithoutReachingTheHandler);
	failures += runTests("aHandledCommandSendsNoDispatcherError",
						  aHandledCommandSendsNoDispatcherError);
	failures += runTests("capLsRepliesWithAnEmptyCapabilityList",
						  capLsRepliesWithAnEmptyCapabilityList);
	failures += runTests("capLsWithAVersionArgumentRepliesTheSameWay",
						  capLsWithAVersionArgumentRepliesTheSameWay);
	failures += runTests("capReqForAnUnsupportedCapabilityIsNeverAcknowledged",
						  capReqForAnUnsupportedCapabilityIsNeverAcknowledged);
	failures += runTests("capEndProducesNoReply",
						  capEndProducesNoReply);
	failures += runTests("bareCapBeforeRegistrationIsSilentlyDropped",
						  bareCapBeforeRegistrationIsSilentlyDropped);
	failures += runTests("bareCapAfterRegistrationIsAnUnknownCommand",
						  bareCapAfterRegistrationIsAnUnknownCommand);
	failures += runTests("pingWithoutAParameterReplies461",
						  pingWithoutAParameterReplies461);
	failures += runTests("pingWithAParameterRepliesWithTheExactEcho",
						  pingWithAParameterRepliesWithTheExactEcho);

	return failures;
}
