#include "Test.hpp"
#include "protocol/DispatchProtocol.hpp"
#include "commands/auth/PassHandler.hpp"
#include "commands/auth/NickHandler.hpp"
#include "commands/auth/UserHandler.hpp"

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

class FakeChannelService : public irc::ChannelService
{
	public:
		void handle(irc::ClientId, const irc::ParsedCommand &) {}
		void removeClient(irc::ClientId) {}
		std::vector<irc::ClientId> peersOf(irc::ClientId) const
		{
			return std::vector<irc::ClientId>();
		}
};

class StubHandler : public irc::CommandHandler
{
	public:
		StubHandler() : callCount(0) {}

		void handle(irc::ClientId, const std::vector<std::string> &params)
		{
			++callCount;
			lastParams = params;
		}

		int callCount;
		std::vector<std::string> lastParams;
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

int linesFor(const RecordingTransport &net, irc::ClientId id)
{
	int count = 0;

	for (std::size_t i = 0; i < net.ids.size(); ++i)
		if (net.ids[i] == id)
			++count;
	return count;
}

bool anyLineContains(const RecordingTransport &net, const std::string &needle)
{
	for (std::size_t i = 0; i < net.lines.size(); ++i)
		if (net.lines[i].find(needle) != std::string::npos)
			return true;
	return false;
}

// PROTO-40: a bounded matrix of malformed/invalid input, all within the
// 512-byte line limit NET-31 already enforces upstream (that bound is
// not this ticket's concern) must each produce a single controlled
// reply or be silently ignored -- never a crash, never an unbounded
// reply loop -- and must never affect a second, well-behaved client
// sharing the same dispatcher/registry.
void malformedInputFromOneClientNeverAffectsASecondHealthyClient()
{
	irc::Dispatcher dispatcher;
	irc::ClientRegistry registry;
	RecordingTransport net;
	irc::DispatchProtocol protocol(dispatcher, registry, net);

	FakePasswordSource source("secret");
	irc::PassHandler passHandler(registry, source, net);
	dispatcher.registerHandler("PASS", &passHandler, 1);
	FakeChannelService channels;
	irc::NickHandler nickHandler(registry, net, channels);
	dispatcher.registerHandler("NICK", &nickHandler, 0);
	irc::UserHandler userHandler(registry, net);
	dispatcher.registerHandler("USER", &userHandler, 4);
	StubHandler privmsg;
	dispatcher.registerHandler("PRIVMSG", &privmsg, 1);

	protocol.connected(1); // the troublemaker
	protocol.connected(2); // the healthy client

	// The matrix: unknown commands, wrong parameter shapes, unexpected
	// prefixes, oversized param counts, and lines that are only noise.
	// Everything here is well within the 512-byte limit on purpose --
	// that bound is NET-31's job, not this test's.
	const char *garbage[] = {
		"",
		":",
		":only-a-prefix",
		"FROBNICATE",
		"FROBNICATE with params anyway",
		"NICK",
		"PASS",
		"USER only one param",
		":fake!forged@identity NICK notActuallyAllowedToForge",
		"a b c d e f g h i j k l m n o p q r s t u v w x y z",
		"    ",
		"PRIVMSG",
		"999NICK",
		":",
	};
	const std::size_t garbageCount = sizeof(garbage) / sizeof(garbage[0]);

	for (std::size_t i = 0; i < garbageCount; ++i)
		protocol.online(1, garbage[i]);

	// Every garbage line produced at most one reply to client 1, and
	// nothing at all was ever sent to client 2 as a side effect.
	CHECK(linesFor(net, 1) <= static_cast<int>(garbageCount));
	CHECK(linesFor(net, 2) == 0);

	// The troublemaker's connection itself is still usable afterwards
	// (no crash, no corrupted dispatcher/registry state)...
	protocol.online(1, "PASS secret");
	protocol.online(1, "NICK alice");
	protocol.online(1, "USER a 0 * :Alice");
	CHECK(net.lines.size() >= 1U);
	CHECK(anyLineContains(net, "001 alice"));

	// ...and the second, healthy client registers and messages
	// successfully, completely unaffected by client 1's garbage.
	protocol.online(2, "PASS secret");
	protocol.online(2, "NICK bob");
	protocol.online(2, "USER b 0 * :Bob");
	CHECK(linesFor(net, 2) >= 1);
	CHECK(anyLineContains(net, "001 bob"));

	protocol.online(2, "PRIVMSG alice :hello despite the chaos");
	CHECK(privmsg.callCount == 1);
	CHECK(privmsg.lastParams.size() == 2U);
	CHECK(privmsg.lastParams[1] == "hello despite the chaos");
}

void aLineAtExactlyTheFifteenParameterLimitIsAcceptedNotTruncated()
{
	irc::Dispatcher dispatcher;
	irc::ClientRegistry registry;
	RecordingTransport net;
	irc::DispatchProtocol protocol(dispatcher, registry, net);
	StubHandler stub;
	dispatcher.registerHandler("MANYPARAMS", &stub);

	protocol.connected(1);
	registry.setRegistered(1);

	std::string line = "MANYPARAMS 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15";
	protocol.online(1, line);

	CHECK(stub.callCount == 1);
	CHECK(stub.lastParams.size() == 15U);
}

}

int main()
{
	int failures = 0;

	failures += runTests(
		"malformedInputFromOneClientNeverAffectsASecondHealthyClient",
		malformedInputFromOneClientNeverAffectsASecondHealthyClient);
	failures += runTests(
		"aLineAtExactlyTheFifteenParameterLimitIsAcceptedNotTruncated",
		aLineAtExactlyTheFifteenParameterLimitIsAcceptedNotTruncated);

	return failures;
}
