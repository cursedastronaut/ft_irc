#include "commands/channel/Names.hpp"

namespace irc
{
void sendNames(ChannelContext &ctx, ClientId actor, const ClientIdentity &identity, const Channel &room)
{
    Parameters fields(1, "=");
    fields.push_back(room.displayName());
    std::string header;
    if (buildNumericReply(353, identity.nickname, fields, "", header) != ReplySuccess)
        return;
    std::string line = header;
    for (Channel::ClientIds::const_iterator it = room.members.begin(); it != room.members.end(); ++it)
    {
        ClientIdentity member;
        if (!ctx.identities.identityOf(*it, member))
            continue;
        std::string nick = (room.isOperator(*it) ? "@" : "") + member.nickname;
        if (line.size() > header.size() && line.size() + 1 + nick.size() > 510)
        {
            ctx.output.enqueueLine(actor, line);
            line = header;
        }
        if (line.size() != header.size())
            line += " ";
        line += nick;
    }
    if (line.size() != header.size())
        ctx.output.enqueueLine(actor, line);
    if (buildNumericReply(366, identity.nickname, Parameters(1, room.displayName()),
                          "End of NAMES list", line) == ReplySuccess)
        ctx.output.enqueueLine(actor, line);
}

void Names::execute(ClientId actor, const ClientIdentity &identity, const Parameters &params)
{
    Channel *room = memberChannel(actor, identity, params[0]);
    if (room)
        sendNames(context, actor, identity, *room);
}
}
