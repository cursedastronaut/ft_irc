#include "commands/channel/ChannelMessage.hpp"

namespace irc
{
void ChannelMessage::execute(ClientId actor, const ClientIdentity &identity, const Parameters &params)
{
    Channel *room = context.registry.find(params[0]);
    if (!room)
    {
        numeric(actor, identity, 403, Parameters(1, params[0]), "No such channel");
        return;
    }
    if (!room->containsMember(actor))
    {
        numeric(actor, identity, 404, Parameters(1, room->displayName()), "Cannot send to channel");
        return;
    }
    if (params[1].empty())
    {
        numeric(actor, identity, 412, Parameters(), "No text to send");
        return;
    }
    std::string event;
    if (buildEventLine(identity, "PRIVMSG", Parameters(1, room->displayName()), params[1], event) != ReplySuccess)
        return;
    for (Channel::ClientIds::const_iterator it = room->members.begin(); it != room->members.end(); ++it)
        if (*it != actor)
            context.output.enqueueLine(*it, event);
}
}
