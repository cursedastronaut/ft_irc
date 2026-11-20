#include "commands/channel/ChannelCommand.hpp"

namespace irc
{
ChannelCommand::ChannelCommand(ChannelContext &ctx, const std::string &name, size_t minimum)
    : context(ctx), name_(name), minimum_(minimum) {}

void ChannelCommand::handle(ClientId actor, const Parameters &params)
{
    ClientIdentity identity;
    if (!context.identities.isRegistered(actor) || !context.identities.identityOf(actor, identity))
    {
        identity.nickname = "*";
        numeric(actor, identity, 451, Parameters(), "You have not registered");
        return;
    }
    if (params.size() < minimum_)
    {
        numeric(actor, identity, 461, Parameters(1, name_), "Not enough parameters");
        return;
    }
    execute(actor, identity, params);
}

void ChannelCommand::numeric(ClientId actor, const ClientIdentity &identity, int code,
                             const Parameters &fields, const std::string &text)
{
    std::string line;
    if (buildNumericReply(code, identity.nickname, fields, text, line) == ReplySuccess)
        context.output.enqueueLine(actor, line);
}

Channel *ChannelCommand::memberChannel(ClientId actor, const ClientIdentity &identity,
                                       const std::string &name)
{
    Channel *channel = context.registry.find(name);
    if (!channel)
        numeric(actor, identity, 403, Parameters(1, name), "No such channel");
    else if (!channel->containsMember(actor))
        numeric(actor, identity, 442, Parameters(1, channel->displayName()), "You're not on that channel");
    else
        return channel;
    return 0;
}

void ChannelCommand::broadcast(const Channel &channel, const std::string &line)
{
    for (Channel::ClientIds::const_iterator it = channel.members.begin(); it != channel.members.end(); ++it)
        context.output.enqueueLine(*it, line);
}
}
