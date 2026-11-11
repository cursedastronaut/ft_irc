#include "../ChannelContractFixtures.hpp"

namespace
{
// Contract-only fake: deliberately NOT an implementation of IRC commands.
class ChannelFixture : public irc::ChannelService
{
public:
    ChannelFixture() : accepted(0), actorPresent(true) {}

    void handle(irc::ClientId actorId, const irc::ParsedCommand &command)
    {
        if (actorId != 1001 || !actorPresent || command.command != "FIXTURE"
            || command.params.size() != 1 || command.params[0] != "valid")
            return;
        ++accepted;
    }

    void removeClient(irc::ClientId actorId)
    {
        if (actorId == 1001)
            actorPresent = false;
    }

    std::vector<irc::ClientId> peersOf(irc::ClientId actorId) const
    {
        std::vector<irc::ClientId> result;
        if (actorId == 1001 && actorPresent)
        {
            result.push_back(1002);
            result.push_back(1003);
        }
        return result;
    }

    unsigned int accepted;
    bool actorPresent;
};
}

irc::ChannelService *makeChannelFixture()
{
    return new ChannelFixture();
}

unsigned int acceptedFixtureCommands(const irc::ChannelService &service)
{
    return static_cast<const ChannelFixture &>(service).accepted;
}
