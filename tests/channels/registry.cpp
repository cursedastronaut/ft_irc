#include "Test.hpp"
#include "channels/ChannelRegistry.hpp"

static void missingLookupsNeverInsert()
{
	irc::ChannelRegistry registry;
	const irc::ChannelRegistry &view = registry;
	CHECK(registry.find("#missing") == 0);
	CHECK(view.find("#missing") == 0);
	CHECK(registry.size() == 0);
	CHECK(!registry.eraseEmpty("#missing"));
	CHECK(registry.size() == 0);
}

static void equivalentNamesReturnOneCanonicalChannel()
{
	irc::ChannelRegistry registry;
	irc::ChannelRegistry::CreateResult first = registry.create("#MiXeD[\\]^");
	CHECK(first.first != 0 && first.second);
	irc::Channel *channel = first.first;
	channel->topic = "keep this topic";
	channel->members.insert(1001);
	irc::ChannelRegistry::CreateResult again = registry.create("#mixed{|}~");
	CHECK(again.first == channel && !again.second);
	CHECK(registry.find("#MIXED{|}~") == channel);
	const irc::ChannelRegistry &view = registry;
	CHECK(view.find("#mixed[\\]^") == channel);
	CHECK(channel->canonicalName() == "#mixed{|}~");
	CHECK(channel->displayName() == "#MiXeD[\\]^");
	CHECK(channel->topic == "keep this topic");
	CHECK(channel->members.count(1001) == 1);
	CHECK(registry.size() == 1);

	CHECK(registry.create("&MiXeD[\\]^").second);
	CHECK(registry.find("&mixed{|}~") != channel);
	CHECK(registry.size() == 2);
}

static void invalidOperationsLeaveExistingStateUntouched()
{
	irc::ChannelRegistry registry;
	irc::Channel *existing = registry.create("#existing").first;
	CHECK(existing != 0);
	existing->topic = "unchanged";
	existing->members.insert(51);
	const irc::ChannelRegistry &view = registry;
	const std::string invalid[] = {
		"", "#", "&", "bad", "+room", "!room", "#bad name", "#bad,name",
		"#bad:name", "#bad\r\n", std::string("#bad\0name", 9),
		"#" + std::string(50, 'a')
	};
	const std::size_t count = sizeof(invalid) / sizeof(invalid[0]);
	for (std::size_t i = 0; i < count; ++i)
	{
		const irc::ChannelRegistry::CreateResult result = registry.create(invalid[i]);
		CHECK(result.first == 0 && !result.second);
		CHECK(registry.find(invalid[i]) == 0);
		CHECK(view.find(invalid[i]) == 0);
		CHECK(!registry.eraseEmpty(invalid[i]));
		CHECK(registry.size() == 1);
		CHECK(registry.find("#existing") == existing);
		CHECK(existing->topic == "unchanged");
		CHECK(existing->members.count(51) == 1);
	}
}

static void nonemptyChannelsCannotBeDeleted()
{
	irc::ChannelRegistry registry;
	irc::Channel *channel = registry.create("#busy").first;
	CHECK(channel != 0);
	channel->members.insert(10);
	channel->members.insert(20);
	channel->operators.insert(10);
	CHECK(!registry.eraseEmpty("#BUSY"));
	channel->members.erase(20);
	CHECK(!registry.eraseEmpty("#busy"));
	CHECK(registry.find("#busy") == channel);
	CHECK(channel->members.count(10) == 1);
	CHECK(channel->operators.count(10) == 1);
	channel->members.clear();
	channel->operators.clear();
	CHECK(registry.eraseEmpty("#BUSY"));
	CHECK(registry.find("#busy") == 0);
	CHECK(registry.size() == 0);
}

static void emptyDeletionDiscardsAllStateAndPreservesOtherChannels()
{
	irc::ChannelRegistry registry;
	irc::Channel *old = registry.create("#First").first;
	irc::Channel *survivor = registry.create("#Other").first;
	CHECK(old != 0 && survivor != 0);
	old->topic = "stale topic";
	old->modes.inviteOnly = true;
	old->modes.topicRestricted = true;
	old->modes.hasKey = true;
	old->modes.key = "stale key";
	old->modes.hasLimit = true;
	old->modes.limit = 3;
	old->invitations.insert(71);
	survivor->members.insert(1001);
	survivor->operators.insert(1001);
	survivor->invitations.insert(1002);
	survivor->topic = "other topic";
	survivor->modes.inviteOnly = true;
	survivor->modes.topicRestricted = true;
	survivor->modes.hasKey = true;
	survivor->modes.key = "other key";
	survivor->modes.hasLimit = true;
	survivor->modes.limit = 8;

	CHECK(registry.eraseEmpty("#FIRST"));
	// old is now invalid and is never dereferenced or compared again.
	CHECK(registry.find("#first") == 0);
	CHECK(registry.size() == 1);
	CHECK(!registry.eraseEmpty("#first"));
	CHECK(registry.find("#other") == survivor);
	CHECK(survivor->members.count(1001) == 1);
	CHECK(survivor->operators.count(1001) == 1);
	CHECK(survivor->invitations.count(1002) == 1);
	CHECK(survivor->topic == "other topic");
	CHECK(survivor->modes.inviteOnly && survivor->modes.topicRestricted);
	CHECK(survivor->modes.hasKey && survivor->modes.key == "other key");
	CHECK(survivor->modes.hasLimit && survivor->modes.limit == 8);

	const irc::ChannelRegistry::CreateResult recreated = registry.create("#fIrSt");
	CHECK(recreated.first != 0 && recreated.second);
	const irc::Channel &fresh = *recreated.first;
	CHECK(fresh.displayName() == "#fIrSt");
	CHECK(fresh.canonicalName() == "#first");
	CHECK(fresh.members.empty() && fresh.operators.empty());
	CHECK(fresh.invitations.empty() && fresh.topic.empty());
	CHECK(!fresh.modes.inviteOnly && !fresh.modes.topicRestricted);
	CHECK(!fresh.modes.hasKey && fresh.modes.key.empty());
	CHECK(!fresh.modes.hasLimit && fresh.modes.limit == 0);
	CHECK(registry.size() == 2);
}

static void insertionsPreserveExistingAddresses()
{
	irc::ChannelRegistry registry;
	irc::Channel *anchor = registry.create("#anchor").first;
	CHECK(anchor != 0);
	anchor->members.insert(73);
	for (unsigned int i = 0; i < 128; ++i)
	{
		std::ostringstream name;
		name << "#channel" << i;
		CHECK(registry.create(name.str()).second);
		CHECK(registry.find("#anchor") == anchor);
		CHECK(anchor->members.count(73) == 1);
	}
	CHECK(registry.size() == 129);
	CHECK(registry.eraseEmpty("#channel64"));
	CHECK(registry.find("#anchor") == anchor);
}

static void registryTests()
{
	missingLookupsNeverInsert();
	equivalentNamesReturnOneCanonicalChannel();
	invalidOperationsLeaveExistingStateUntouched();
	nonemptyChannelsCannotBeDeleted();
	emptyDeletionDiscardsAllStateAndPreservesOtherChannels();
	insertionsPreserveExistingAddresses();
}

int main()
{
	return runTests("CHAN-03 channel registry", registryTests);
}
