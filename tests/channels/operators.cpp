#include "Test.hpp"
#include "channels/Channel.hpp"

static void operatorsRequireMembershipInThatChannel()
{
    irc::Channel alpha("#alpha"), beta("#beta");
    CHECK(!alpha.grantOperator(1001));
    CHECK(alpha.operators.empty());
    CHECK(alpha.addMember(1001));
    CHECK(alpha.addMember(1002));
    CHECK(beta.addMember(1001));
    CHECK(!alpha.isOperator(1001));
    CHECK(alpha.grantOperator(1001));
    CHECK(!alpha.grantOperator(1001));
    CHECK(alpha.isOperator(1001));
    CHECK(!beta.isOperator(1001));
    CHECK(!alpha.revokeOperator(9999));
    CHECK(alpha.isOperator(1001));
    CHECK(alpha.revokeOperator(1001));
    CHECK(!alpha.revokeOperator(1001));
    CHECK(!alpha.isOperator(1001));
    CHECK(!alpha.isOperator(1002)); // No automatic last-operator succession.
    CHECK(alpha.grantOperator(1001));
    CHECK(alpha.removeMember(1001));
    CHECK(!alpha.isOperator(1001));
    CHECK(alpha.operators.empty());
    CHECK(!alpha.grantOperator(1001));
    CHECK(alpha.containsMember(1002));
    CHECK(!alpha.isOperator(1002));
    CHECK(beta.containsMember(1001));
}

int main()
{
    return runTests("CHAN-05 channel-scoped operators", operatorsRequireMembershipInThatChannel);
}
