#include "Test.hpp"
#include "commands/auth/PassHandler.hpp"
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

void correctPasswordMarksClientAccepted()
{
	irc::ClientRegistry registry;
	FakePasswordSource source("secret");
	RecordingTransport net;
	irc::PassHandler handler(registry, source, net);
	irc::ClientRecord out;
	std::vector<std::string> params;

	registry.create(1);
	params.push_back("secret");
	handler.handle(1, params);

	CHECK(registry.find(1, out) == true);
	CHECK(out.passwordAccepted == true);
}

void wrongPasswordMarksClientNotAccepted()
{
	irc::ClientRegistry registry;
	FakePasswordSource source("secret");
	RecordingTransport net;
	irc::PassHandler handler(registry, source, net);
	irc::ClientRecord out;
	std::vector<std::string> params;

	registry.create(1);
	params.push_back("wrong");
	handler.handle(1, params);

	CHECK(registry.find(1, out) == true);
	CHECK(out.passwordAccepted == false);
}

void presentButEmptyPasswordIsComparedNormallyNotTreatedAsMissing()
{
	irc::ClientRegistry registry;
	FakePasswordSource source("secret");
	RecordingTransport net;
	irc::PassHandler handler(registry, source, net);
	irc::ClientRecord out;
	std::vector<std::string> params;

	registry.create(1);
	params.push_back("");
	handler.handle(1, params);

	CHECK(registry.find(1, out) == true);
	CHECK(out.passwordAccepted == false);
}

void resendingPassWithAWrongPasswordDowngradesAPreviousAcceptance()
{
	irc::ClientRegistry registry;
	FakePasswordSource source("secret");
	RecordingTransport net;
	irc::PassHandler handler(registry, source, net);
	irc::ClientRecord out;
	std::vector<std::string> correctParams;
	std::vector<std::string> wrongParams;

	registry.create(1);
	correctParams.push_back("secret");
	handler.handle(1, correctParams);
	wrongParams.push_back("wrong");
	handler.handle(1, wrongParams);

	CHECK(registry.find(1, out) == true);
	CHECK(out.passwordAccepted == false);
}

void handleOnAnUnregisteredClientDoesNotInsertAFakeRecord()
{
	irc::ClientRegistry registry;
	FakePasswordSource source("secret");
	RecordingTransport net;
	irc::PassHandler handler(registry, source, net);
	irc::ClientRecord out;
	std::vector<std::string> params;

	params.push_back("secret");
	handler.handle(999, params);

	CHECK(registry.find(999, out) == false);
}

void passAfterRegistrationRepliesWith462AndKeepsAcceptance()
{
	irc::ClientRegistry registry;
	FakePasswordSource source("secret");
	RecordingTransport net;
	irc::PassHandler handler(registry, source, net);
	irc::ClientRecord out;
	std::vector<std::string> params;

	registry.create(1);
	registry.setPasswordAccepted(1, true);
	registry.setNickname(1, "bob");
	registry.setRegistered(1);
	params.push_back("wrong");
	handler.handle(1, params);

	CHECK(registry.find(1, out) == true);
	CHECK(out.passwordAccepted == true);
	CHECK(net.lines.size() == 1);
	CHECK(net.ids[0] == 1);
	CHECK(net.lines[0] == ":ircserv 462 bob :Unauthorized command (already registered)");
}

void correctPassAfterRegistrationAlsoReplies462()
{
	irc::ClientRegistry registry;
	FakePasswordSource source("secret");
	RecordingTransport net;
	irc::PassHandler handler(registry, source, net);
	irc::ClientRecord out;
	std::vector<std::string> params;

	registry.create(1);
	registry.setRegistered(1);
	params.push_back("secret");
	handler.handle(1, params);

	CHECK(registry.find(1, out) == true);
	CHECK(out.passwordAccepted == false);
	CHECK(net.lines.size() == 1);
	CHECK(net.lines[0] == ":ircserv 462 * :Unauthorized command (already registered)");
}

void passBeforeRegistrationSendsNothing()
{
	irc::ClientRegistry registry;
	FakePasswordSource source("secret");
	RecordingTransport net;
	irc::PassHandler handler(registry, source, net);
	std::vector<std::string> params;

	registry.create(1);
	params.push_back("wrong");
	handler.handle(1, params);
	params[0] = "secret";
	handler.handle(1, params);

	CHECK(net.lines.empty());
}

}

int main()
{
	int failures = 0;

	failures += runTests("correctPasswordMarksClientAccepted",
						  correctPasswordMarksClientAccepted);
	failures += runTests("wrongPasswordMarksClientNotAccepted",
						  wrongPasswordMarksClientNotAccepted);
	failures += runTests("presentButEmptyPasswordIsComparedNormallyNotTreatedAsMissing",
						  presentButEmptyPasswordIsComparedNormallyNotTreatedAsMissing);
	failures += runTests("resendingPassWithAWrongPasswordDowngradesAPreviousAcceptance",
						  resendingPassWithAWrongPasswordDowngradesAPreviousAcceptance);
	failures += runTests("handleOnAnUnregisteredClientDoesNotInsertAFakeRecord",
						  handleOnAnUnregisteredClientDoesNotInsertAFakeRecord);

	failures += runTests("passAfterRegistrationRepliesWith462AndKeepsAcceptance",
						  passAfterRegistrationRepliesWith462AndKeepsAcceptance);
	failures += runTests("correctPassAfterRegistrationAlsoReplies462",
						  correctPassAfterRegistrationAlsoReplies462);
	failures += runTests("passBeforeRegistrationSendsNothing",
						  passBeforeRegistrationSendsNothing);

	return failures;
}
