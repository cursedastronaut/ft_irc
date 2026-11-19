#include "Test.hpp"
#include "contracts/IrcCaseMapping.hpp"

namespace
{

void ordinaryLettersFoldToLowercase()
{
	CHECK(irc::canonicalizeIrcName("Bob") == "bob");
	CHECK(irc::canonicalizeIrcName("BOB") == "bob");
	CHECK(irc::canonicalizeIrcName("bob") == "bob");
}

void specialCharactersMapToTheirRfc1459Equivalent()
{
	CHECK(irc::canonicalizeIrcName("[") == "{");
	CHECK(irc::canonicalizeIrcName("]") == "}");
	CHECK(irc::canonicalizeIrcName("\\") == "|");
	CHECK(irc::canonicalizeIrcName("^") == "~");
}

void equivalentSpellingsMapToTheSameKey()
{
	CHECK(irc::canonicalizeIrcName("Bob") == irc::canonicalizeIrcName("BOB"));
	CHECK(irc::canonicalizeIrcName("Bob[x]")
		  == irc::canonicalizeIrcName("BOB[X]"));
	CHECK(irc::canonicalizeIrcName("Bob[x]")
		  == irc::canonicalizeIrcName("bob{x}"));
	CHECK(irc::canonicalizeIrcName("a\\b") == irc::canonicalizeIrcName("A|B"));
	CHECK(irc::canonicalizeIrcName("nick^")
		  == irc::canonicalizeIrcName("NICK~"));
}

void distinctPermittedNicknamesStayDistinct()
{
	CHECK(irc::canonicalizeIrcName("bob") != irc::canonicalizeIrcName("rob"));
	CHECK(irc::canonicalizeIrcName("bob")
		  != irc::canonicalizeIrcName("bobby"));
	CHECK(irc::canonicalizeIrcName("bob{x}")
		  != irc::canonicalizeIrcName("bob{y}"));
}

void untouchedCharactersPassThrough()
{
	CHECK(irc::canonicalizeIrcName("nick-9") == "nick-9");
	CHECK(irc::canonicalizeIrcName("") == "");
}

}

int main()
{
	int failures = 0;

	failures += runTests("ordinaryLettersFoldToLowercase",
						  ordinaryLettersFoldToLowercase);
	failures += runTests("specialCharactersMapToTheirRfc1459Equivalent",
						  specialCharactersMapToTheirRfc1459Equivalent);
	failures += runTests("equivalentSpellingsMapToTheSameKey",
						  equivalentSpellingsMapToTheSameKey);
	failures += runTests("distinctPermittedNicknamesStayDistinct",
						  distinctPermittedNicknamesStayDistinct);
	failures += runTests("untouchedCharactersPassThrough",
						  untouchedCharactersPassThrough);

	return failures;
}
