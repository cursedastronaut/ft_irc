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

void correctPasswordMarksClientAccepted()
{
	irc::ClientRegistry registry;
	FakePasswordSource source("secret");
	irc::PassHandler handler(registry, source);
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
	irc::PassHandler handler(registry, source);
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
	irc::PassHandler handler(registry, source);
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
	irc::PassHandler handler(registry, source);
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
	irc::PassHandler handler(registry, source);
	irc::ClientRecord out;
	std::vector<std::string> params;

	params.push_back("secret");
	handler.handle(999, params);

	CHECK(registry.find(999, out) == false);
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

	return failures;
}
