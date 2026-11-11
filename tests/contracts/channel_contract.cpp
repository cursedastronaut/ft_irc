#include "Test.hpp"
#include "ChannelContractFixtures.hpp"
#include <memory>

static void identitySnapshotsAndFailedLookups()
{
    std::auto_ptr<irc::IdentityLookup> lookup(makeIdentityFixture());
    ClientIdentity first;
    CHECK(lookup->identityOf(1001, first));
    first.nickname = "changed locally";
    ClientIdentity again;
    CHECK(lookup->identityOf(1001, again));
    CHECK(again.nickname == "Op[One]");
    CHECK(!lookup->identityOf(9999, first));
    CHECK(first.nickname == "changed locally");
    irc::ClientId id = 7777;
    CHECK(!lookup->clientIdForNickname("absent", id));
    CHECK(id == 7777);
    CHECK(lookup->clientIdForNickname("OP{ONE}", id));
    CHECK(id == 1001);
    CHECK(lookup->isRegistered(1001));
    CHECK(!lookup->isRegistered(9999));
}

static void channelBoundaryAndFailureExample()
{
    std::auto_ptr<irc::ChannelService> channel(makeChannelFixture());
    irc::ParsedCommand command;
    command.command = "FIXTURE";
    channel->handle(1001, command); // Missing parameter: no fake state mutation.
    CHECK(acceptedFixtureCommands(*channel) == 0);
    command.params.push_back("valid");
    channel->handle(9999, command);
    CHECK(acceptedFixtureCommands(*channel) == 0);
    channel->handle(1001, command);
    CHECK(acceptedFixtureCommands(*channel) == 1);
    std::vector<irc::ClientId> peers = channel->peersOf(1001);
    CHECK(peers.size() == 2 && peers[0] == 1002 && peers[1] == 1003);
    peers.clear();
    CHECK(channel->peersOf(1001).size() == 2);
    channel->removeClient(1001);
    channel->removeClient(1001);
    CHECK(channel->peersOf(1001).empty());
}

int main()
{
    return runTests("CORE-03 identity snapshots", identitySnapshotsAndFailedLookups)
        + runTests("CORE-03 separate channel stub", channelBoundaryAndFailureExample);
}
