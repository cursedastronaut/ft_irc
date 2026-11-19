#include "Test.hpp"
#include "protocol/PongHandler.hpp"

namespace
{

void pongIsAcceptedAsANoOpThroughDispatcher()
{
	irc::Dispatcher dispatcher;
	irc::PongHandler pong;
	dispatcher.registerHandler("PONG", &pong);

	irc::ParsedCommand command;
	command.command = "PONG";
	command.params.push_back("server1");

	irc::DispatchResult result = dispatcher.dispatch(1, command);

	CHECK(result == irc::DispatchHandled);
}

void repeatedPongCallsNeverThrowRegardlessOfActorOrParams()
{
	irc::PongHandler pong;

	for (int i = 0; i < 100; ++i)
	{
		std::vector<std::string> params;
		if (i % 2 == 0)
			params.push_back("server1");
		pong.handle(static_cast<irc::ClientId>(i), params);
	}

	CHECK(true);
}

}

int main()
{
	int failures = 0;

	failures += runTests("pongIsAcceptedAsANoOpThroughDispatcher",
						  pongIsAcceptedAsANoOpThroughDispatcher);
	failures += runTests("repeatedPongCallsNeverThrowRegardlessOfActorOrParams",
						  repeatedPongCallsNeverThrowRegardlessOfActorOrParams);

	return failures;
}
