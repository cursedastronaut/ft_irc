#include "../ChannelContractFixtures.hpp"
#include "contracts/IrcCaseMapping.hpp"

namespace
{
class IdentityFixture : public irc::IdentityLookup
{
public:
    bool identityOf(irc::ClientId id, ClientIdentity &out) const
    {
        if (id != 1001)
            return false;
        ClientIdentity snapshot;
        snapshot.nickname = "Op[One]";
        snapshot.username = "operator";
        snapshot.host = "fixture.invalid";
        out = snapshot;
        return true;
    }

    bool clientIdForNickname(const std::string &nickname, irc::ClientId &out) const
    {
        if (irc::canonicalizeIrcName(nickname) != "op{one}")
            return false;
        out = 1001;
        return true;
    }

    bool isRegistered(irc::ClientId id) const
    {
        return id == 1001;
    }
};
}

irc::IdentityLookup *makeIdentityFixture()
{
    return new IdentityFixture();
}
