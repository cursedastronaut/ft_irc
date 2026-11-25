#include "Test.hpp"
#include "commands/auth/Welcome.hpp"
#include "protocol/ClientRegistry.hpp"

namespace
{

class FakeTransport : public transport
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

void sendWelcomeEnqueuesTheExactWireFormat()
{
	irc::ClientRegistry registry;
	FakeTransport net;

	registry.create(1);
	registry.setNickname(1, "bob");
	registry.setUser(1, "bob", "Bob Real");
	registry.setPasswordAccepted(1, true);

	irc::sendWelcome(1, registry, net);

	CHECK(net.lines.size() == 1);
	CHECK(net.ids[0] == 1);
	CHECK(net.lines[0] == ":ircserv 001 bob :Welcome to the Internet Relay "
						   "Network bob!bob@ircserv");
}

void sendWelcomeOnAnUnknownClientDoesNothing()
{
	irc::ClientRegistry registry;
	FakeTransport net;

	irc::sendWelcome(999, registry, net);

	CHECK(net.lines.empty());
}

void sendWelcomeOnANotYetFullyRegisteredClientDoesNothing()
{
	irc::ClientRegistry registry;
	FakeTransport net;

	registry.create(1);
	registry.setNickname(1, "bob");
	registry.setUser(1, "bob", "Bob Real");
	// passwordAccepted never set -> not registered yet

	irc::sendWelcome(1, registry, net);

	CHECK(net.lines.empty());
}

}

int main()
{
	int failures = 0;

	failures += runTests("sendWelcomeEnqueuesTheExactWireFormat",
						  sendWelcomeEnqueuesTheExactWireFormat);
	failures += runTests("sendWelcomeOnAnUnknownClientDoesNothing",
						  sendWelcomeOnAnUnknownClientDoesNothing);
	failures += runTests("sendWelcomeOnANotYetFullyRegisteredClientDoesNothing",
						  sendWelcomeOnANotYetFullyRegisteredClientDoesNothing);

	return failures;
}
