#include "Test.hpp"
#include "channels/Channel.hpp"

#include <limits>
#include <map>

static void emptyValueNeedsNoServer()
{
	const irc::Channel channel("#room");
	CHECK(channel.canonicalName() == "#room");
	CHECK(channel.displayName() == "#room");
	CHECK(channel.members.empty());
	CHECK(channel.operators.empty());
	CHECK(channel.invitations.empty());
	CHECK(channel.topic.empty());
	CHECK(!channel.modes.inviteOnly);
	CHECK(!channel.modes.topicRestricted);
	CHECK(!channel.modes.hasKey);
	CHECK(channel.modes.key.empty());
	CHECK(!channel.modes.hasLimit);
	CHECK(channel.modes.limit == 0);
}

static void membershipSurvivesNicknameChanges()
{
	const irc::ClientId actor = 1001;
	const irc::ClientId replacement = 1002;
	std::map<irc::ClientId, std::string> nicknames;
	irc::Channel channel("#room");
	nicknames[actor] = "before";
	channel.members.insert(actor);
	channel.operators.insert(actor);
	channel.invitations.insert(actor);

	nicknames[actor] = "after";
	nicknames[replacement] = "before";
	CHECK(channel.members.count(actor) == 1);
	CHECK(channel.operators.count(actor) == 1);
	CHECK(channel.invitations.count(actor) == 1);
	CHECK(channel.members.count(replacement) == 0);
	CHECK(channel.operators.count(replacement) == 0);
	CHECK(channel.invitations.count(replacement) == 0);
	CHECK(channel.members.size() == 1);
}

static void copiesOwnIndependentState()
{
	irc::Channel original("#room");
	const irc::ClientId highId = std::numeric_limits<irc::ClientId>::max();
	original.members.insert(highId);
	original.operators.insert(highId);
	original.invitations.insert(42);
	original.topic = "original topic";
	original.modes.inviteOnly = true;
	original.modes.topicRestricted = true;
	original.modes.hasKey = true;
	original.modes.key = "channel key";
	original.modes.hasLimit = true;
	original.modes.limit = 4;

	irc::Channel copy(original);
	CHECK(copy.members.count(highId) == 1);
	CHECK(copy.operators.count(highId) == 1);
	CHECK(copy.invitations.count(42) == 1);
	CHECK(copy.topic == "original topic");
	CHECK(copy.modes.inviteOnly && copy.modes.topicRestricted);
	CHECK(copy.modes.hasKey && copy.modes.key == "channel key");
	CHECK(copy.modes.hasLimit && copy.modes.limit == 4);
	copy.members.clear();
	copy.operators.clear();
	copy.invitations.clear();
	copy.topic = "changed";
	copy.modes.key.clear();
	CHECK(original.members.count(highId) == 1);
	CHECK(original.operators.count(highId) == 1);
	CHECK(original.invitations.count(42) == 1);
	CHECK(original.topic == "original topic");
	CHECK(original.modes.key == "channel key");
}

static void modelTests()
{
	emptyValueNeedsNoServer();
	membershipSurvivesNicknameChanges();
	copiesOwnIndependentState();
}

int main()
{
	return runTests("CHAN-01 channel value model", modelTests);
}
