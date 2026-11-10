#include "Test.hpp"
#include "protocol/Parser.hpp"

namespace
{

void simpleCommandWithOneParam()
{
	irc::ParsedCommand out;
	irc::ParseResult result = irc::parseCommand("NICK bob", out);

	CHECK(result == irc::ParseSuccess);
	CHECK(out.command == "NICK");
	CHECK(out.params.size() == 1);
	CHECK(out.params[0] == "bob");
}

void prefixIsSkippedAndNotPartOfCommand()
{
	irc::ParsedCommand out;
	irc::ParseResult result = irc::parseCommand(":nick CMD arg1 arg2", out);

	CHECK(result == irc::ParseSuccess);
	CHECK(out.command == "CMD");
	CHECK(out.params.size() == 2);
	CHECK(out.params[0] == "arg1");
	CHECK(out.params[1] == "arg2");
}

void trailingParamKeepsInternalSpacesWithoutColon()
{
	irc::ParsedCommand out;
	irc::ParseResult result =
		irc::parseCommand("PRIVMSG #general :salut les amis", out);

	CHECK(result == irc::ParseSuccess);
	CHECK(out.command == "PRIVMSG");
	CHECK(out.params.size() == 2);
	CHECK(out.params[0] == "#general");
	CHECK(out.params[1] == "salut les amis");
}

void emptyTrailingParamIsKeptAsEmptyString()
{
	irc::ParsedCommand out;
	irc::ParseResult result = irc::parseCommand("NICK :", out);

	CHECK(result == irc::ParseSuccess);
	CHECK(out.command == "NICK");
	CHECK(out.params.size() == 1);
	CHECK(out.params[0] == "");
}

void commandWithoutAnyParamIsValid()
{
	irc::ParsedCommand out;
	irc::ParseResult result = irc::parseCommand("QUIT", out);

	CHECK(result == irc::ParseSuccess);
	CHECK(out.command == "QUIT");
	CHECK(out.params.size() == 0);
}

void commandWithPrefixAndNoParamIsValid()
{
	irc::ParsedCommand out;
	irc::ParseResult result = irc::parseCommand(":nick QUIT", out);

	CHECK(result == irc::ParseSuccess);
	CHECK(out.command == "QUIT");
	CHECK(out.params.size() == 0);
}

void leadingSpacesBeforeCommandAreSkipped()
{
	irc::ParsedCommand out;
	irc::ParseResult result = irc::parseCommand("   NICK bob", out);

	CHECK(result == irc::ParseSuccess);
	CHECK(out.command == "NICK");
	CHECK(out.params.size() == 1);
	CHECK(out.params[0] == "bob");
}

void multipleSpacesBetweenPrefixAndCommandAreSkipped()
{
	irc::ParsedCommand out;
	irc::ParseResult result = irc::parseCommand(":prefix    NICK   bob", out);

	CHECK(result == irc::ParseSuccess);
	CHECK(out.command == "NICK");
	CHECK(out.params.size() == 1);
	CHECK(out.params[0] == "bob");
}

void multipleSpacesBetweenParamsAreCollapsed()
{
	irc::ParsedCommand out;
	irc::ParseResult result = irc::parseCommand("NICK   bob   baz", out);

	CHECK(result == irc::ParseSuccess);
	CHECK(out.command == "NICK");
	CHECK(out.params.size() == 2);
	CHECK(out.params[0] == "bob");
	CHECK(out.params[1] == "baz");
}

void trailingSpacesDoNotCreateAnEmptyParam()
{
	irc::ParsedCommand out;
	irc::ParseResult result = irc::parseCommand("NICK bob   ", out);

	CHECK(result == irc::ParseSuccess);
	CHECK(out.command == "NICK");
	CHECK(out.params.size() == 1);
	CHECK(out.params[0] == "bob");
}

void trailingSpacesOnlyAfterCommandGiveNoParam()
{
	irc::ParsedCommand out;
	irc::ParseResult result = irc::parseCommand("QUIT    ", out);

	CHECK(result == irc::ParseSuccess);
	CHECK(out.command == "QUIT");
	CHECK(out.params.size() == 0);
}

void paramLimitOfFifteenIsEnforced()
{
	std::string line = "CMD";
	for (int i = 0; i < 16; ++i)
		line += " p";
	irc::ParsedCommand out;
	irc::ParseResult result = irc::parseCommand(line, out);

	CHECK(result == irc::ParseError);
}

void emptyLineIsIgnoredNotAnError()
{
	irc::ParsedCommand out;
	irc::ParseResult result = irc::parseCommand("", out);

	CHECK(result == irc::ParseIgnore);
}

void lineWithOnlySpacesIsAnError()
{
	irc::ParsedCommand out;
	irc::ParseResult result = irc::parseCommand(" ", out);

	CHECK(result == irc::ParseError);
}

void prefixWithoutCommandAfterIsAnError()
{
	irc::ParsedCommand out;
	irc::ParseResult result = irc::parseCommand(":  ", out);

	CHECK(result == irc::ParseError);
}

}

int main()
{
	int failures = 0;

	failures += runTests("simpleCommandWithOneParam", simpleCommandWithOneParam);
	failures += runTests("prefixIsSkippedAndNotPartOfCommand",
						  prefixIsSkippedAndNotPartOfCommand);
	failures += runTests("trailingParamKeepsInternalSpacesWithoutColon",
						  trailingParamKeepsInternalSpacesWithoutColon);
	failures += runTests("emptyTrailingParamIsKeptAsEmptyString",
						  emptyTrailingParamIsKeptAsEmptyString);
	failures += runTests("commandWithoutAnyParamIsValid",
						  commandWithoutAnyParamIsValid);
	failures += runTests("commandWithPrefixAndNoParamIsValid",
						  commandWithPrefixAndNoParamIsValid);
	failures += runTests("leadingSpacesBeforeCommandAreSkipped",
						  leadingSpacesBeforeCommandAreSkipped);
	failures += runTests("multipleSpacesBetweenPrefixAndCommandAreSkipped",
						  multipleSpacesBetweenPrefixAndCommandAreSkipped);
	failures += runTests("multipleSpacesBetweenParamsAreCollapsed",
						  multipleSpacesBetweenParamsAreCollapsed);
	failures += runTests("trailingSpacesDoNotCreateAnEmptyParam",
						  trailingSpacesDoNotCreateAnEmptyParam);
	failures += runTests("trailingSpacesOnlyAfterCommandGiveNoParam",
						  trailingSpacesOnlyAfterCommandGiveNoParam);
	failures += runTests("paramLimitOfFifteenIsEnforced",
						  paramLimitOfFifteenIsEnforced);
	failures += runTests("emptyLineIsIgnoredNotAnError",
						  emptyLineIsIgnoredNotAnError);
	failures += runTests("lineWithOnlySpacesIsAnError",
						  lineWithOnlySpacesIsAnError);
	failures += runTests("prefixWithoutCommandAfterIsAnError",
						  prefixWithoutCommandAfterIsAnError);

	return failures;
}
