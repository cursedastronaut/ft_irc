#include "Test.hpp"
#include "contracts/Protocol.hpp"

namespace
{

class StubProtocol : public protocol
{
public:
	StubProtocol() : lastId(0), lastLine(), lastReason(),
					  connectedCalls(0), disconnectedCalls(0) {}

	void connected(irc::ClientId id)
	{
		lastId = id;
		++connectedCalls;
	}

	void online(irc::ClientId id, const std::string &line)
	{
		lastId = id;
		lastLine = line;
	}

	void disconnected(irc::ClientId id, const std::string &reason)
	{
		lastId = id;
		lastReason = reason;
		++disconnectedCalls;
	}

	irc::ClientId lastId;
	std::string lastLine;
	std::string lastReason;
	int connectedCalls;
	int disconnectedCalls;
};

class StubTransport : public transport
{
public:
	ClientIdentity returnIdentity(irc::ClientId id) const
	{
		(void)id;
		ClientIdentity identity;
		identity.nickname = "nick";
		identity.username = "user";
		identity.host = "host";
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
		(void)id;
		if (line.find('\r') != std::string::npos
			|| line.find('\n') != std::string::npos)
			return Rejected;
		return Queued;
	}
};

void protocolSinkReceivesEventsThroughBasePointer()
{
	StubProtocol stub;
	protocol *sink = &stub;

	sink->connected(42);
	CHECK(stub.connectedCalls == 1);
	CHECK(stub.lastId == 42);

	sink->online(42, "NICK bob");
	CHECK(stub.lastLine == "NICK bob");

	sink->disconnected(42, "Connection reset");
	CHECK(stub.disconnectedCalls == 1);
	CHECK(stub.lastReason == "Connection reset");
}

void transportEnqueueLineRejectsEmbeddedCrlf()
{
	StubTransport stub;
	transport *net = &stub;

	CHECK(net->enqueueLine(7, "PRIVMSG #chan :hello") == Queued);
	CHECK(net->enqueueLine(7, "evil\r\ninjected") == Rejected);
}

void transportReturnsIdentitySnapshotByValue()
{
	StubTransport stub;
	transport *net = &stub;

	ClientIdentity identity = net->returnIdentity(7);
	CHECK(identity.nickname == "nick");
	CHECK(identity.username == "user");
	CHECK(identity.host == "host");
}

}

int main()
{
	int failures = 0;

	failures += runTests("protocolSinkReceivesEventsThroughBasePointer",
						  protocolSinkReceivesEventsThroughBasePointer);
	failures += runTests("transportEnqueueLineRejectsEmbeddedCrlf",
						  transportEnqueueLineRejectsEmbeddedCrlf);
	failures += runTests("transportReturnsIdentitySnapshotByValue",
						  transportReturnsIdentitySnapshotByValue);

	return failures;
}
