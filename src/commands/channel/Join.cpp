#include "commands/channel/Join.hpp"
#include "commands/channel/Names.hpp"

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
    int denied = 0;
    if (channel->modes.inviteOnly && !channel->hasInvitation(actor))
        denied = 473;
    else if (channel->modes.hasKey && (params.size() < 2 || params[1] != channel->modes.key))
        denied = 475;
    else if (channel->modes.hasLimit && channel->members.size() >= channel->modes.limit)
        denied = 471;
    if (denied)
    {
        numeric(actor, identity, denied, Parameters(1, channel->displayName()), "Channel admission refused");
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

    channel->consumeInvitation(actor);
    std::string event;
    if (buildEventLine(identity, "JOIN", Parameters(), channel->displayName(), event) == ReplySuccess)
        broadcast(*channel, event);
    bool empty = channel->topic.empty();
    numeric(actor, identity, empty ? 331 : 332, Parameters(1, channel->displayName()),
            empty ? "No topic is set" : channel->topic);
    sendNames(context, actor, identity, *channel);
}
}
