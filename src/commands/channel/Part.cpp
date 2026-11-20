#include "commands/channel/Part.hpp"

namespace irc
{
void Part::execute(ClientId actor, const ClientIdentity &identity, const Parameters &params)
{
    Channel *channel = memberChannel(actor, identity, params[0]);
    if (!channel)
        return;
    std::string line;
    if (params.size() == 1)
    {
        // The shared builder always adds a trailing field. For a reasonless
        // PART, use the channel as that field, then serialize it as a middle.
        if (buildEventLine(identity, "PART", Parameters(), channel->displayName(), line) != ReplySuccess)
            return;
        line.erase(line.size() - channel->displayName().size() - 1, 1);
    }
    else if (buildEventLine(identity, "PART", Parameters(1, channel->displayName()), params[1], line) != ReplySuccess)
        return;
    broadcast(*channel, line);
    channel->removeMember(actor);
    context.registry.eraseEmpty(params[0]);
}
}
