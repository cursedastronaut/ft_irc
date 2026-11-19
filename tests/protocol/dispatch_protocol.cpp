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

void connectedCreatesARegistryEntry()
{
	irc::Dispatcher dispatcher;
	irc::ClientRegistry registry;
	irc::DispatchProtocol protocol(dispatcher, registry);
	irc::ClientRecord record;

	CHECK(registry.find(1, record) == false);
	protocol.connected(1);
	CHECK(registry.find(1, record) == true);
}

void disconnectedRemovesTheRegistryEntry()
{
	irc::Dispatcher dispatcher;
	irc::ClientRegistry registry;
	irc::DispatchProtocol protocol(dispatcher, registry);
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
	irc::DispatchProtocol protocol(dispatcher, registry);

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
	irc::DispatchProtocol protocol(dispatcher, registry);

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
	irc::DispatchProtocol protocol(dispatcher, registry);

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
	irc::DispatchProtocol protocol(dispatcher, registry);

	protocol.online(1, "PASS secret");

	CHECK(pass.callCount == 1);
}

void anUnknownClientNeverConnectedCannotReachARegularHandler()
{
	irc::Dispatcher dispatcher;
	irc::ClientRegistry registry;
	StubHandler privmsg;
	dispatcher.registerHandler("PRIVMSG", &privmsg);
	irc::DispatchProtocol protocol(dispatcher, registry);

	protocol.online(1, "PRIVMSG bob :hi");

	CHECK(privmsg.callCount == 0);
}

void onlineOnAnEmptyLineIsIgnoredSilently()
{
	irc::Dispatcher dispatcher;
	irc::ClientRegistry registry;
	StubHandler nick;
	dispatcher.registerHandler("NICK", &nick);
	irc::DispatchProtocol protocol(dispatcher, registry);

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
	irc::DispatchProtocol protocol(dispatcher, registry);

	protocol.connected(1);
	protocol.online(1, ":");

	CHECK(nick.callCount == 0);
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

	return failures;
}
