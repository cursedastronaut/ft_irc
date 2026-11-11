#include "Test.hpp"
#include "channels/ChannelRegistry.hpp"

static void fakeDisconnectClearsAllIdentityState()
{
    irc::ChannelRegistry registry;
    irc::Channel *alpha = registry.create("#alpha").first;
    irc::Channel *beta = registry.create("#beta").first;
    irc::Channel *gamma = registry.create("#gamma").first;
    irc::Channel *zeta = registry.create("#zeta").first;
    irc::Channel *empty = registry.create("#empty").first;
    CHECK(alpha && beta && gamma && zeta && empty);
    alpha->addMember(1001);
    alpha->addMember(1002);
    alpha->grantOperator(1001);
    alpha->addInvitation(1003);
    beta->addMember(1001);
    beta->grantOperator(1001);
    beta->topic = "discard with channel";
    beta->modes.hasKey = true;
    beta->modes.key = "discard-channel-key";
    beta->addInvitation(1004);
    gamma->addMember(1003);
    gamma->grantOperator(1003);
    gamma->addInvitation(1001); // Actor never joined this channel.
    gamma->topic = "untouched";
    zeta->addMember(1001);
    empty->addInvitation(1001);

    // Notification recipients are captured by the caller, before teardown.
    const std::vector<irc::ClientId> before = registry.peersOf(1001);
    CHECK(before.size() == 1 && before[0] == 1002);
    registry.removeClient(1001);
    CHECK(registry.size() == 2);
    CHECK(registry.find("#beta") == 0);
    CHECK(registry.find("#zeta") == 0);
    CHECK(registry.find("#empty") == 0);
    CHECK(registry.find("#alpha") == alpha);
    CHECK(registry.find("#gamma") == gamma);
    CHECK(!alpha->containsMember(1001));
    CHECK(!alpha->hasInvitation(1001));
    CHECK(alpha->operators.count(1001) == 0);
    CHECK(alpha->containsMember(1002));
    CHECK(!alpha->isOperator(1002));
    CHECK(alpha->hasInvitation(1003));
    CHECK(gamma->containsMember(1003) && gamma->isOperator(1003));
    CHECK(!gamma->hasInvitation(1001));
    CHECK(gamma->topic == "untouched");
    CHECK(registry.peersOf(1001).empty());
    CHECK(before.size() == 1 && before[0] == 1002);

    registry.removeClient(1001);
    registry.removeClient(9999);
    CHECK(registry.size() == 2);
    CHECK(alpha->members.size() == 1 && gamma->members.size() == 1);
    CHECK(!alpha->hasInvitation(1005)); // A new stable ID inherits nothing.
    CHECK(!gamma->hasInvitation(1005));
    const irc::ChannelRegistry::CreateResult recreated = registry.create("#BETA");
    CHECK(recreated.first != 0 && recreated.second);
    CHECK(recreated.first->topic.empty());
    CHECK(recreated.first->invitations.empty());
    CHECK(!recreated.first->modes.hasKey && recreated.first->modes.key.empty());
}

int main()
{
    return runTests("CHAN-08 idempotent disconnect cleanup", fakeDisconnectClearsAllIdentityState);
}
