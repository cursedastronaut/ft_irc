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

    if (channel->modes.inviteOnly && !channel->isOperator(actor))
    {
        numeric(actor, identity, 482, Parameters(1, channel->displayName()), "You're not channel operator");
        return;
    }
    ClientIdentity invited;
    if (!context.identities.identityOf(target, invited))
        return;
    std::string event;
    if (buildEventLine(identity, "INVITE", Parameters(1, invited.nickname),
                       channel->displayName(), event) != ReplySuccess)
        return;
    bool added = channel->addInvitation(target);
    if (context.output.enqueueLine(target, event) != Queued)
    {
        if (added)
            channel->revokeInvitation(target);
        return;
    }
    fields[0] = invited.nickname;
    numeric(actor, identity, 341, fields, "Inviting");
}
}
