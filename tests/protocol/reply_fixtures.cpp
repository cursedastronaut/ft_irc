#include "Test.hpp"
#include "protocol/Reply.hpp"

namespace
{

void welcomeNumericReplyProducesExactWireFormat()
{
	std::string out;
	irc::ReplyResult result = irc::buildNumericReply(1, "bob",
		std::vector<std::string>(),
		"Welcome to the Internet Relay Network bob!bob@localhost", out);

	CHECK(result == irc::ReplySuccess);
	CHECK(out == ":ircserv 001 bob :Welcome to the Internet Relay Network "
				 "bob!bob@localhost");
}

void unknownCommandErrorReplyProducesExactWireFormat()
{
	std::vector<std::string> params;
	params.push_back("bob");
	params.push_back("WHOIS");
	std::string out;
	irc::ReplyResult result =
		irc::buildNumericReply(421, "bob", params, "Unknown command", out);

	CHECK(result == irc::ReplySuccess);
	CHECK(out == ":ircserv 421 bob bob WHOIS :Unknown command");
}

void nicknameInUseErrorReplyProducesExactWireFormat()
{
	std::vector<std::string> params;
	params.push_back("*");
	params.push_back("bob");
	std::string out;
	irc::ReplyResult result = irc::buildNumericReply(433, "*", params,
		"Nickname is already in use", out);

	CHECK(result == irc::ReplySuccess);
	CHECK(out == ":ircserv 433 * * bob :Nickname is already in use");
}

void eventLineWithFullUserPrefixProducesExactWireFormat()
{
	ClientIdentity identity;
	identity.nickname = "alice";
	identity.username = "alice";
	identity.host = "localhost";
	std::vector<std::string> params;
	params.push_back("#general");
	std::string out;
	irc::ReplyResult result = irc::buildEventLine(identity, "PRIVMSG",
		params, "hello everyone", out);

	CHECK(result == irc::ReplySuccess);
	CHECK(out == ":alice!alice@localhost PRIVMSG #general :hello everyone");
}

void successfulNumericReplyNeverContainsCarriageReturnOrLineFeed()
{
	std::vector<std::string> params;
	params.push_back("bob");
	std::string out;
	irc::ReplyResult result =
		irc::buildNumericReply(1, "bob", params, "welcome", out);

	CHECK(result == irc::ReplySuccess);
	CHECK(out.find('\r') == std::string::npos);
	CHECK(out.find('\n') == std::string::npos);
}

void successfulEventLineNeverContainsCarriageReturnOrLineFeed()
{
	ClientIdentity identity;
	identity.nickname = "alice";
	identity.username = "alice";
	identity.host = "localhost";
	std::string out;
	irc::ReplyResult result = irc::buildEventLine(identity, "PRIVMSG",
		std::vector<std::string>(), "hello", out);

	CHECK(result == irc::ReplySuccess);
	CHECK(out.find('\r') == std::string::npos);
	CHECK(out.find('\n') == std::string::npos);
}

void attemptedCommandInjectionInTextIsRejectedNotEmitted()
{
	std::string out;
	irc::ReplyResult result = irc::buildNumericReply(1, "bob",
		std::vector<std::string>(), "welcome\r\nQUIT :bye", out);

	CHECK(result == irc::ReplyError);
}

}

int main()
{
	int failures = 0;

	failures += runTests("welcomeNumericReplyProducesExactWireFormat",
						  welcomeNumericReplyProducesExactWireFormat);
	failures += runTests("unknownCommandErrorReplyProducesExactWireFormat",
						  unknownCommandErrorReplyProducesExactWireFormat);
	failures += runTests("nicknameInUseErrorReplyProducesExactWireFormat",
						  nicknameInUseErrorReplyProducesExactWireFormat);
	failures += runTests("eventLineWithFullUserPrefixProducesExactWireFormat",
						  eventLineWithFullUserPrefixProducesExactWireFormat);
	failures += runTests(
		"successfulNumericReplyNeverContainsCarriageReturnOrLineFeed",
		successfulNumericReplyNeverContainsCarriageReturnOrLineFeed);
	failures += runTests(
		"successfulEventLineNeverContainsCarriageReturnOrLineFeed",
		successfulEventLineNeverContainsCarriageReturnOrLineFeed);
	failures += runTests("attemptedCommandInjectionInTextIsRejectedNotEmitted",
						  attemptedCommandInjectionInTextIsRejectedNotEmitted);

	return failures;
}
