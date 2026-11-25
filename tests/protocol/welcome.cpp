#include "Test.hpp"
#include "commands/auth/Welcome.hpp"
#include "protocol/ClientRegistry.hpp"

namespace
{

class FakeTransport : public transport
{
	public:
		FakeTransport(const std::string &host) : _host(host) {}

		ClientIdentity returnIdentity(irc::ClientId id) const
		{
			ClientIdentity identity;

			(void)id;
			identity.host = _host;
			return identity;
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

	private:
		std::string _host;
};

void sendWelcomeEnqueuesTheExactWireFormat()
{
	irc::ClientRegistry registry;
	FakeTransport net("localhost");

	registry.create(1);
	registry.setNickname(1, "bob");
	registry.setUser(1, "bob", "Bob Real");

	irc::sendWelcome(1, registry, net);

	CHECK(net.lines.size() == 1);
	CHECK(net.ids[0] == 1);
	CHECK(net.lines[0] == ":ircserv 001 bob :Welcome to the Internet Relay "
						   "Network bob!bob@localhost");
}

void sendWelcomeOnAnUnknownClientDoesNothing()
{
	irc::ClientRegistry registry;
	FakeTransport net("localhost");

	irc::sendWelcome(999, registry, net);

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

	return failures;
}
