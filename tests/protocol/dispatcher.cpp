#include "Test.hpp"
#include "protocol/Dispatcher.hpp"

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

void dispatchCallsTheRegisteredHandlerWithCorrectActorAndParams()
{
	irc::Dispatcher dispatcher;
	StubHandler foo;
	dispatcher.registerHandler("FOO", &foo);

	irc::ParsedCommand command;
	command.command = "FOO";
	command.params.push_back("arg1");
	command.params.push_back("arg2");

	irc::DispatchResult result = dispatcher.dispatch(42, command);

	CHECK(result == irc::DispatchHandled);
	CHECK(foo.callCount == 1);
	CHECK(foo.lastActor == 42UL);
	CHECK(foo.lastParams.size() == 2U);
	CHECK(foo.lastParams[0] == "arg1");
	CHECK(foo.lastParams[1] == "arg2");
}

void eachHandlerOnlyReceivesItsOwnCommand()
{
	irc::Dispatcher dispatcher;
	StubHandler foo;
	StubHandler bar;
	dispatcher.registerHandler("FOO", &foo);
	dispatcher.registerHandler("BAR", &bar);

	irc::ParsedCommand fooCommand;
	fooCommand.command = "FOO";
	irc::ParsedCommand barCommand;
	barCommand.command = "BAR";

	dispatcher.dispatch(1, fooCommand);
	dispatcher.dispatch(2, barCommand);

	CHECK(foo.callCount == 1);
	CHECK(foo.lastActor == 1UL);
	CHECK(bar.callCount == 1);
	CHECK(bar.lastActor == 2UL);
}

void dispatchOnUnknownCommandDoesNotCallAnyHandlerAndSignalsIt()
{
	irc::Dispatcher dispatcher;
	StubHandler foo;
	dispatcher.registerHandler("FOO", &foo);

	irc::ParsedCommand command;
	command.command = "QUUX";

	irc::DispatchResult result = dispatcher.dispatch(1, command);

	CHECK(result == irc::DispatchUnknownCommand);
	CHECK(foo.callCount == 0);
}

void dispatchWithNoParamsForwardsAnEmptyVector()
{
	irc::Dispatcher dispatcher;
	StubHandler foo;
	dispatcher.registerHandler("FOO", &foo);

	irc::ParsedCommand command;
	command.command = "FOO";

	dispatcher.dispatch(7, command);

	CHECK(foo.callCount == 1);
	CHECK(foo.lastParams.empty());
}

void dispatchRejectsMissingParametersWithoutCallingTheHandler()
{
	irc::Dispatcher dispatcher;
	StubHandler foo;
	dispatcher.registerHandler("FOO", &foo, 2);

	irc::ParsedCommand command;
	command.command = "FOO";
	command.params.push_back("arg1");

	CHECK(dispatcher.dispatch(1, command) == irc::DispatchNeedMoreParams);
	CHECK(foo.callCount == 0);

	command.params.push_back("arg2");

	CHECK(dispatcher.dispatch(1, command) == irc::DispatchHandled);
	CHECK(foo.callCount == 1);
}

}

int main()
{
	int failures = 0;

	failures += runTests("dispatchCallsTheRegisteredHandlerWithCorrectActorAndParams",
						  dispatchCallsTheRegisteredHandlerWithCorrectActorAndParams);
	failures += runTests("eachHandlerOnlyReceivesItsOwnCommand",
						  eachHandlerOnlyReceivesItsOwnCommand);
	failures += runTests("dispatchOnUnknownCommandDoesNotCallAnyHandlerAndSignalsIt",
						  dispatchOnUnknownCommandDoesNotCallAnyHandlerAndSignalsIt);
	failures += runTests("dispatchWithNoParamsForwardsAnEmptyVector",
						  dispatchWithNoParamsForwardsAnEmptyVector);
	failures += runTests("dispatchRejectsMissingParametersWithoutCallingTheHandler",
						  dispatchRejectsMissingParametersWithoutCallingTheHandler);

	return failures;
}
