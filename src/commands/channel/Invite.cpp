#include "commands/channel/Invite.hpp"

namespace irc
{
void Invite::execute(ClientId actor, const ClientIdentity &identity, const Parameters &params)
{
    Channel *channel = memberChannel(actor, identity, params[1]);
    if (!channel)
        return;
    ClientId target;
    if (!context.identities.clientIdForNickname(params[0], target) || !context.identities.isRegistered(target))
    {
        numeric(actor, identity, 401, Parameters(1, params[0]), "No such nick");
        return;
    }
    Parameters fields(1, params[0]);
    fields.push_back(channel->displayName());
    if (channel->containsMember(target))
    {
        numeric(actor, identity, 443, fields, "is already on channel");
        return;
    }
    channel->addInvitation(target);
}
}
