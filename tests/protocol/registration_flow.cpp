#include "Test.hpp"
#include "commands/auth/PassHandler.hpp"
#include "commands/auth/NickHandler.hpp"
#include "commands/auth/UserHandler.hpp"
#include "protocol/ClientRegistry.hpp"

namespace
{

class FakePasswordSource : public PasswordSource
{
	public:
		FakePasswordSource(const std::string &password) : _password(password) {}
		const std::string &getPassword() const { return _password; }

	private:
		std::string _password;
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

std::vector<std::string> one(const std::string &value)
{
	std::vector<std::string> params;

	params.push_back(value);
	return params;
}

std::vector<std::string> userParams(const std::string &username,
									  const std::string &realname)
{
	std::vector<std::string> params;

	params.push_back(username);
	params.push_back("h");
	params.push_back("s");
	params.push_back(realname);
	return params;
}

bool isWelcomeLine(const std::string &line)
{
	return line.find(":ircserv 001 ") == 0;
}

int countWelcomeLines(const RecordingTransport &net)
{
	int count = 0;
	std::vector<std::string>::const_iterator it;

	for (it = net.lines.begin(); it != net.lines.end(); ++it)
		if (isWelcomeLine(*it))
			++count;
	return count;
}

void completingPassNickUserInThatOrderSendsExactlyOneWelcome()
{
	irc::ClientRegistry registry;
	FakePasswordSource source("secret");
	RecordingTransport net;
	irc::PassHandler passHandler(registry, source, net);
	irc::NickHandler nickHandler(registry, net);
	irc::UserHandler userHandler(registry, net);

	registry.create(1);
	passHandler.handle(1, one("secret"));
	nickHandler.handle(1, one("bob"));
	userHandler.handle(1, userParams("bob", "Bob Real"));

	CHECK(countWelcomeLines(net) == 1);
}

void completingTheSameThreeInAReversedOrderAlsoSendsExactlyOneWelcome()
{
	irc::ClientRegistry registry;
	FakePasswordSource source("secret");
	RecordingTransport net;
	irc::PassHandler passHandler(registry, source, net);
	irc::NickHandler nickHandler(registry, net);
	irc::UserHandler userHandler(registry, net);

	registry.create(1);
	userHandler.handle(1, userParams("bob", "Bob Real"));
	nickHandler.handle(1, one("bob"));
	passHandler.handle(1, one("secret"));

	CHECK(countWelcomeLines(net) == 1);
}

void aFailedPasswordAfterTheOtherTwoPrerequisitesSendsNoWelcome()
{
	irc::ClientRegistry registry;
	FakePasswordSource source("secret");
	RecordingTransport net;
	irc::PassHandler passHandler(registry, source, net);
	irc::NickHandler nickHandler(registry, net);
	irc::UserHandler userHandler(registry, net);

	registry.create(1);
	nickHandler.handle(1, one("bob"));
	userHandler.handle(1, userParams("bob", "Bob Real"));
	passHandler.handle(1, one("wrong"));

	CHECK(countWelcomeLines(net) == 0);
}

void retryingAFailedPasswordWithTheCorrectOneStillCompletesRegistrationOnce()
{
	irc::ClientRegistry registry;
	FakePasswordSource source("secret");
	RecordingTransport net;
	irc::PassHandler passHandler(registry, source, net);
	irc::NickHandler nickHandler(registry, net);
	irc::UserHandler userHandler(registry, net);

	registry.create(1);
	nickHandler.handle(1, one("bob"));
	userHandler.handle(1, userParams("bob", "Bob Real"));
	passHandler.handle(1, one("wrong"));
	passHandler.handle(1, one("secret"));

	CHECK(countWelcomeLines(net) == 1);
}

void renamingAfterRegistrationDoesNotResendTheWelcome()
{
	irc::ClientRegistry registry;
	FakePasswordSource source("secret");
	RecordingTransport net;
	irc::PassHandler passHandler(registry, source, net);
	irc::NickHandler nickHandler(registry, net);
	irc::UserHandler userHandler(registry, net);

	registry.create(1);
	passHandler.handle(1, one("secret"));
	nickHandler.handle(1, one("bob"));
	userHandler.handle(1, userParams("bob", "Bob Real"));
	nickHandler.handle(1, one("alice"));

	CHECK(countWelcomeLines(net) == 1);
}

}

int main()
{
	int failures = 0;

	failures += runTests("completingPassNickUserInThatOrderSendsExactlyOneWelcome",
						  completingPassNickUserInThatOrderSendsExactlyOneWelcome);
	failures += runTests(
		"completingTheSameThreeInAReversedOrderAlsoSendsExactlyOneWelcome",
		completingTheSameThreeInAReversedOrderAlsoSendsExactlyOneWelcome);
	failures += runTests("aFailedPasswordAfterTheOtherTwoPrerequisitesSendsNoWelcome",
						  aFailedPasswordAfterTheOtherTwoPrerequisitesSendsNoWelcome);
	failures += runTests(
		"retryingAFailedPasswordWithTheCorrectOneStillCompletesRegistrationOnce",
		retryingAFailedPasswordWithTheCorrectOneStillCompletesRegistrationOnce);
	failures += runTests("renamingAfterRegistrationDoesNotResendTheWelcome",
						  renamingAfterRegistrationDoesNotResendTheWelcome);

	return failures;
}
