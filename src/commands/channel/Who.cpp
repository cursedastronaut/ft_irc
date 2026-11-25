#include "commands/channel/Who.hpp"

namespace irc
{
void Who::execute(ClientId actor, const ClientIdentity &identity, const Parameters &params)
{
    const Channel *room = context.registry.find(params[0]);
    if (room && room->containsMember(actor))
    {
        for (Channel::ClientIds::const_iterator it = room->members.begin(); it != room->members.end(); ++it)
        {
            ClientIdentity member;
            if (!context.identities.identityOf(*it, member))
                continue;
            Parameters fields;
            fields.push_back(room->displayName());
            fields.push_back(member.username);
            fields.push_back(member.host);
            fields.push_back("ircserv");
            fields.push_back(member.nickname);
            fields.push_back(room->isOperator(*it) ? "H@" : "H");
            // The channel identity contract doesn't expose the real name.
            numeric(actor, identity, 352, fields, "0");
        }
    }
    numeric(actor, identity, 315, Parameters(1, params[0]), "End of WHO list");
}
}
