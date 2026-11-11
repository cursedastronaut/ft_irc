#include "Test.hpp"
#include "channels/Channel.hpp"

static void membershipIsIdempotentAndChannelScoped()
{
    irc::Channel first("#first");
    irc::Channel second("#second");
    CHECK(!first.containsMember(1001));
    CHECK(!first.removeMember(1001));
    CHECK(first.addMember(1001));
    CHECK(!first.addMember(1001));
    CHECK(first.members.size() == 1);
    CHECK(first.containsMember(1001));
    CHECK(first.operators.empty()); // Data insertion is not a successful JOIN.
    CHECK(!second.containsMember(1001));
    CHECK(first.addMember(1002));
    first.operators.insert(1001);
    first.invitations.insert(1001);
    CHECK(!first.removeMember(9999));
    CHECK(first.members.size() == 2);
    CHECK(first.operators.count(1001) == 1);
    CHECK(first.removeMember(1001));
    CHECK(!first.containsMember(1001));
    CHECK(first.operators.count(1001) == 0);
    CHECK(first.invitations.count(1001) == 1);
    CHECK(first.containsMember(1002));
    CHECK(!first.removeMember(1001));
    CHECK(first.members.size() == 1);
}

int main()
{
    return runTests("CHAN-04 idempotent membership", membershipIsIdempotentAndChannelScoped);
}
