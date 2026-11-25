#include "Test.hpp"
#include "commands/auth/UserHandler.hpp"
#include "protocol/ClientRegistry.hpp"

namespace
{

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

std::vector<std::string> fourParams(const std::string &username,
									 const std::string &hostname,
									 const std::string &servername,
									 const std::string &realname)
{
	std::vector<std::string> params;

	params.push_back(username);
	params.push_back(hostname);
	params.push_back(servername);
	params.push_back(realname);
	return params;
}

void successfulUserStoresUsernameAndRealnameWithNoReply()
{
	irc::ClientRegistry registry;
	RecordingTransport net;
	irc::UserHandler handler(registry, net);
	irc::ClientRecord out;

	registry.create(1);
	handler.handle(1, fourParams("bob", "bobhost", "localhost", "Bob Real"));

	CHECK(net.lines.empty());
	CHECK(registry.find(1, out) == true);
	CHECK(out.username == "bob");
	CHECK(out.realname == "Bob Real");
	CHECK(out.userSet == true);
}

void realnameWithInternalSpacesIsStoredIntact()
{
	irc::ClientRegistry registry;
	RecordingTransport net;
	irc::UserHandler handler(registry, net);
	irc::ClientRecord out;

	registry.create(1);
	handler.handle(1, fourParams("bob", "bobhost", "localhost",
								  "Multi Word Real Name"));

	CHECK(registry.find(1, out) == true);
	CHECK(out.realname == "Multi Word Real Name");
}

void resendingUserBeforeRegistrationOverwritesPreviousValues()
{
	irc::ClientRegistry registry;
	RecordingTransport net;
	irc::UserHandler handler(registry, net);
	irc::ClientRecord out;

	registry.create(1);
	handler.handle(1, fourParams("bob", "h", "s", "First Name"));
	handler.handle(1, fourParams("alice", "h", "s", "Second Name"));

	CHECK(net.lines.empty());
	CHECK(registry.find(1, out) == true);
	CHECK(out.username == "alice");
	CHECK(out.realname == "Second Name");
}

void userAfterRegistrationReplies462AndKeepsPreviousValues()
{
	irc::ClientRegistry registry;
	RecordingTransport net;
	irc::UserHandler handler(registry, net);
	irc::ClientRecord out;

	registry.create(1);
	registry.setUser(1, "bob", "Bob Real");
	registry.setPasswordAccepted(1, true);
	registry.setNickname(1, "bob");
	registry.setRegistered(1);
	handler.handle(1, fourParams("mallory", "h", "s", "Mallory Real"));

	CHECK(registry.find(1, out) == true);
	CHECK(out.username == "bob");
	CHECK(out.realname == "Bob Real");
	CHECK(net.lines.size() == 1);
	CHECK(net.ids[0] == 1);
	CHECK(net.lines[0] == ":ircserv 462 bob :Unauthorized command (already registered)");
}

void handleOnAnUnregisteredClientDoesNotInsertAFakeRecord()
{
	irc::ClientRegistry registry;
	RecordingTransport net;
	irc::UserHandler handler(registry, net);
	irc::ClientRecord out;

	handler.handle(999, fourParams("bob", "h", "s", "Bob Real"));

	CHECK(net.lines.empty());
	CHECK(registry.find(999, out) == false);
}

}

int main()
{
	int failures = 0;

	failures += runTests("successfulUserStoresUsernameAndRealnameWithNoReply",
						  successfulUserStoresUsernameAndRealnameWithNoReply);
	failures += runTests("realnameWithInternalSpacesIsStoredIntact",
						  realnameWithInternalSpacesIsStoredIntact);
	failures += runTests("resendingUserBeforeRegistrationOverwritesPreviousValues",
						  resendingUserBeforeRegistrationOverwritesPreviousValues);
	failures += runTests("userAfterRegistrationReplies462AndKeepsPreviousValues",
						  userAfterRegistrationReplies462AndKeepsPreviousValues);
	failures += runTests("handleOnAnUnregisteredClientDoesNotInsertAFakeRecord",
						  handleOnAnUnregisteredClientDoesNotInsertAFakeRecord);

	return failures;
}
