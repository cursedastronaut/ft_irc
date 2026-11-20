#include "commands/channel/Topic.hpp"

namespace irc
{
void Topic::execute(ClientId actor, const ClientIdentity &identity, const Parameters &params)
{
    Channel *channel = memberChannel(actor, identity, params[0]);
    if (!channel)
        return;
    Parameters fields(1, channel->displayName());
    if (params.size() == 1)
    {
        bool empty = channel->topic.empty();
        numeric(actor, identity, empty ? 331 : 332, fields, empty ? "No topic is set" : channel->topic);
        return;
    }

    if (params[1].empty())
        return;
    if (channel->modes.topicRestricted && !channel->isOperator(actor))
    {
        numeric(actor, identity, 482, fields, "You're not channel operator");
        return;
    }
    std::string event;
    if (buildEventLine(identity, "TOPIC", fields, params[1], event) == ReplySuccess)
    {
        channel->topic = params[1];
        broadcast(*channel, event);
    }
}
}
