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

void onlineDispatchesAValidLineToTheRegisteredHandler()
{
	irc::Dispatcher dispatcher;
	StubHandler foo;
	dispatcher.registerHandler("FOO", &foo);
	irc::DispatchProtocol protocol(dispatcher);

	protocol.online(42, "FOO arg1 arg2");

	CHECK(foo.callCount == 1);
	CHECK(foo.lastActor == 42UL);
	CHECK(foo.lastParams.size() == 2U);
	CHECK(foo.lastParams[0] == "arg1");
	CHECK(foo.lastParams[1] == "arg2");
}

void onlineOnAnUnknownCommandCallsNoHandlerAndDoesNotThrow()
{
	irc::Dispatcher dispatcher;
	StubHandler foo;
	dispatcher.registerHandler("FOO", &foo);
	irc::DispatchProtocol protocol(dispatcher);

	protocol.online(1, "QUUX arg1");

	CHECK(foo.callCount == 0);
}

void onlineOnAnEmptyLineIsIgnoredSilently()
{
	irc::Dispatcher dispatcher;
	StubHandler foo;
	dispatcher.registerHandler("FOO", &foo);
	irc::DispatchProtocol protocol(dispatcher);

	protocol.online(1, "");

	CHECK(foo.callCount == 0);
}

void onlineOnAMalformedPrefixIsIgnoredSilently()
{
	irc::Dispatcher dispatcher;
	StubHandler foo;
	dispatcher.registerHandler("FOO", &foo);
	irc::DispatchProtocol protocol(dispatcher);

	protocol.online(1, ":");

	CHECK(foo.callCount == 0);
}

}

int main()
{
	int failures = 0;

	failures += runTests("onlineDispatchesAValidLineToTheRegisteredHandler",
						  onlineDispatchesAValidLineToTheRegisteredHandler);
	failures += runTests("onlineOnAnUnknownCommandCallsNoHandlerAndDoesNotThrow",
						  onlineOnAnUnknownCommandCallsNoHandlerAndDoesNotThrow);
	failures += runTests("onlineOnAnEmptyLineIsIgnoredSilently",
						  onlineOnAnEmptyLineIsIgnoredSilently);
	failures += runTests("onlineOnAMalformedPrefixIsIgnoredSilently",
						  onlineOnAMalformedPrefixIsIgnoredSilently);

	return failures;
}
