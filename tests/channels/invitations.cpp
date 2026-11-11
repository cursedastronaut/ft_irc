#include "Test.hpp"
#include "channels/Channel.hpp"
#include <map>

static void invitationsFollowIdentityAndSuccessfulAdmission()
{
    irc::Channel channel("#room");
    std::map<irc::ClientId, std::string> names;
    names[1001] = "before";
    CHECK(!channel.hasInvitation(1001));
    CHECK(channel.addInvitation(1001));
    CHECK(!channel.addInvitation(1001));
    names[1001] = "after";
    names[1002] = "before";
    CHECK(channel.hasInvitation(1001));
    CHECK(!channel.hasInvitation(1002));
    CHECK(!channel.consumeInvitation(1001));
    CHECK(channel.hasInvitation(1001));
    CHECK(channel.addMember(1001));
    CHECK(channel.consumeInvitation(1001));
    CHECK(!channel.consumeInvitation(1001));
    CHECK(!channel.hasInvitation(1001));
    CHECK(channel.containsMember(1001));
    CHECK(channel.addInvitation(1003));
    CHECK(!channel.revokeInvitation(9999));
    CHECK(channel.hasInvitation(1003));
    CHECK(channel.revokeInvitation(1003));
    CHECK(!channel.revokeInvitation(1003));
    CHECK(!channel.hasInvitation(1003));
}

int main()
{
    return runTests("CHAN-07 stable invitations", invitationsFollowIdentityAndSuccessfulAdmission);
}
