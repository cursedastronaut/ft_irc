#include "Test.hpp"
#include "net/ServerConfig.hpp"

#include <cstddef>
#include <string>

namespace
{

void checkInvalidPort(const char *input)
{
	bool rejected = false;
	try
	{
		irc::port_parser(input);
	}
	catch (const std::exception &)
	{
		rejected = true;
	}
	CHECK(rejected);
}

static void portParserAcceptsBoundsAndRejectsInvalidInputs()
{
	CHECK(irc::port_parser("1") == 1);
	CHECK(irc::port_parser("65535") == 65535);
	CHECK(irc::port_parser("6667") == 6667);

	const char *invalid[] = {
		"0",
		"65536",
		"-1",
		"+6667",
		"9999999999999999999999999999",
		"2147483648",
		"6667a",
		"",
		0
	};
	const std::size_t count = sizeof(invalid) / sizeof(invalid[0]);
	for (std::size_t i = 0; i < count; ++i)
		checkInvalidPort(invalid[i]);
}

static void passwordAndServerConfigCopyPreserveRawSecret()
{
	bool empty_rejected = false;
	try
	{
		irc::psw_parser("");
	}
	catch (const std::exception &)
	{
		empty_rejected = true;
	}
	CHECK(empty_rejected);

	bool null_rejected = false;
	try
	{
		irc::psw_parser(0);
	}
	catch (const std::exception &)
	{
		null_rejected = true;
	}
	CHECK(null_rejected);

	const std::string raw_secret = irc::psw_parser("s3cr3t!\t_bytes");
	irc::ServerConfig serv(irc::port_parser("6667"), raw_secret);
	CHECK(serv.getPort() == 6667);
	CHECK(serv.getPSW() == raw_secret);
	CHECK(serv.getClientNb() == 0);

	irc::ServerConfig copy_serv(serv);
	CHECK(copy_serv.getPort() == 6667);
	CHECK(copy_serv.getPSW() == raw_secret);
	CHECK(copy_serv.getClientNb() == 0);
}

static void clientCounterEnforcesBoundsAndPrefixPostfixSemantics()
{
	irc::ServerConfig serv(6667, "fixture-secret");

	bool underflow = false;
	try
	{
		--serv;
	}
	catch (const irc::ServerConfig::ClientUnderFlow &)
	{
		underflow = true;
	}
	CHECK(underflow);

	irc::ServerConfig &ref_inc = ++serv;
	CHECK(&ref_inc == &serv);
	CHECK(serv.getClientNb() == 1);

	irc::ServerConfig old_inc = serv++;
	CHECK(old_inc.getClientNb() == 1);
	CHECK(serv.getClientNb() == 2);

	irc::ServerConfig old_dec = serv--;
	CHECK(old_dec.getClientNb() == 2);
	CHECK(serv.getClientNb() == 1);

	irc::ServerConfig &ref_dec = --serv;
	CHECK(&ref_dec == &serv);
	CHECK(serv.getClientNb() == 0);

	for (std::size_t i = 0; i < MAX_CLIENT; ++i)
		++serv;
	CHECK(serv.getClientNb() == MAX_CLIENT);

	bool overflow = false;
	try
	{
		serv++;
	}
	catch (const irc::ServerConfig::ClientOutOfRange &)
	{
		overflow = true;
	}
	CHECK(overflow);
	CHECK(serv.getClientNb() == MAX_CLIENT);
}

static void configTests()
{
	portParserAcceptsBoundsAndRejectsInvalidInputs();
	passwordAndServerConfigCopyPreserveRawSecret();
	clientCounterEnforcesBoundsAndPrefixPostfixSemantics();
}

} // namespace

int main()
{
	return runTests("NET-06..07 config and parsers", configTests);
}
