#include "commands/channel/Join.hpp"

namespace irc
{
void Join::execute(ClientId actor, const ClientIdentity &identity, const Parameters &params)
{
    ChannelRegistry::CreateResult entry = context.registry.create(params[0]);
    Channel *channel = entry.first;
    if (!channel)
    {
        numeric(actor, identity, 476, Parameters(1, "*"), "Invalid channel name");
        return;
    }
    if (channel->containsMember(actor))
        return;
    if (channel->modes.inviteOnly || channel->modes.hasKey || channel->modes.hasLimit)
    {
        // Restricted admission is implemented with the respective MODE tasks.
        int code = channel->modes.inviteOnly ? 473 : (channel->modes.hasKey ? 475 : 471);
        numeric(actor, identity, code, Parameters(1, channel->displayName()), "Channel admission restricted");
        return;
    }
    try
    {
        bool first = channel->members.empty();
        channel->addMember(actor);
        if (first)
            channel->grantOperator(actor);
    }
    catch (...)
    {
        channel->removeMember(actor);
        if (entry.second)
            context.registry.eraseEmpty(params[0]);
        throw;
    }
}
}
