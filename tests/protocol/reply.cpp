#include "Test.hpp"
#include "protocol/Reply.hpp"

namespace
{

void numericReplyWithParamsProducesExactBytes()
{
	std::vector<std::string> params;
	params.push_back("#chan");
	std::string out;

	irc::ReplyResult result = irc::buildNumericReply(1, "bob", params,
													   "Welcome", out);

	CHECK(result == irc::ReplySuccess);
	CHECK(out == ":ircserv 001 bob #chan :Welcome");
}

void numericReplyWithoutParamsProducesExactBytes()
{
	std::vector<std::string> empty;
	std::string out;

	irc::ReplyResult result = irc::buildNumericReply(2, "bob", empty,
													   "no params here", out);

	CHECK(result == irc::ReplySuccess);
	CHECK(out == ":ircserv 002 bob :no params here");
}

void numericReplyPadsToThreeDigits()
{
	std::vector<std::string> empty;
	std::string out;

	irc::buildNumericReply(42, "bob", empty, "text", out);

	CHECK(out == ":ircserv 042 bob :text");
}

void numericReplyWithMultipleParamsSeparatesThemWithSpaces()
{
	std::vector<std::string> params;
	params.push_back("#chan");
	params.push_back("extra");
	std::string out;

	irc::buildNumericReply(3, "bob", params, "text", out);

	CHECK(out == ":ircserv 003 bob #chan extra :text");
}

void numericReplyRejectsCRLFInText()
{
	std::vector<std::string> empty;
	std::string out = "untouched";

	irc::ReplyResult result = irc::buildNumericReply(1, "bob", empty,
													   "bad\r\ntext", out);

	CHECK(result == irc::ReplyError);
	CHECK(out == "untouched");
}

void numericReplyRejectsCRLFInParams()
{
	std::vector<std::string> params;
	params.push_back("bad\r\nparam");
	std::string out = "untouched";

	irc::ReplyResult result = irc::buildNumericReply(1, "bob", params,
													   "text", out);

	CHECK(result == irc::ReplyError);
	CHECK(out == "untouched");
}

void eventLineWithParamsProducesExactBytes()
{
	ClientIdentity identity;
	identity.nickname = "bob";
	identity.username = "u";
	identity.host = "h";
	std::vector<std::string> params;
	params.push_back("#chan");
	std::string out;

	irc::ReplyResult result = irc::buildEventLine(identity, "PRIVMSG",
													 params, "hello", out);

	CHECK(result == irc::ReplySuccess);
	CHECK(out == ":bob!u@h PRIVMSG #chan :hello");
}

void eventLineWithoutParamsProducesExactBytes()
{
	ClientIdentity identity;
	identity.nickname = "bob";
	identity.username = "u";
	identity.host = "h";
	std::vector<std::string> empty;
	std::string out;

	irc::ReplyResult result = irc::buildEventLine(identity, "QUIT", empty,
													 "bye", out);

	CHECK(result == irc::ReplySuccess);
	CHECK(out == ":bob!u@h QUIT :bye");
}

void eventLineRejectsCRLFInText()
{
	ClientIdentity identity;
	identity.nickname = "bob";
	identity.username = "u";
	identity.host = "h";
	std::vector<std::string> empty;
	std::string out = "untouched";

	irc::ReplyResult result = irc::buildEventLine(identity, "QUIT", empty,
													 "bad\r\ntext", out);

	CHECK(result == irc::ReplyError);
	CHECK(out == "untouched");
}

void eventLineRejectsCRLFInParams()
{
	ClientIdentity identity;
	identity.nickname = "bob";
	identity.username = "u";
	identity.host = "h";
	std::vector<std::string> params;
	params.push_back("bad\r\nparam");
	std::string out = "untouched";

	irc::ReplyResult result = irc::buildEventLine(identity, "PRIVMSG",
													 params, "text", out);

	CHECK(result == irc::ReplyError);
	CHECK(out == "untouched");
}

}

int main()
{
	int failures = 0;

	failures += runTests("numericReplyWithParamsProducesExactBytes",
						  numericReplyWithParamsProducesExactBytes);
	failures += runTests("numericReplyWithoutParamsProducesExactBytes",
						  numericReplyWithoutParamsProducesExactBytes);
	failures += runTests("numericReplyPadsToThreeDigits",
						  numericReplyPadsToThreeDigits);
	failures += runTests("numericReplyWithMultipleParamsSeparatesThemWithSpaces",
						  numericReplyWithMultipleParamsSeparatesThemWithSpaces);
	failures += runTests("numericReplyRejectsCRLFInText",
						  numericReplyRejectsCRLFInText);
	failures += runTests("numericReplyRejectsCRLFInParams",
						  numericReplyRejectsCRLFInParams);
	failures += runTests("eventLineWithParamsProducesExactBytes",
						  eventLineWithParamsProducesExactBytes);
	failures += runTests("eventLineWithoutParamsProducesExactBytes",
						  eventLineWithoutParamsProducesExactBytes);
	failures += runTests("eventLineRejectsCRLFInText",
						  eventLineRejectsCRLFInText);
	failures += runTests("eventLineRejectsCRLFInParams",
						  eventLineRejectsCRLFInParams);

	return failures;
}
