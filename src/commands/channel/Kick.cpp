#include "commands/channel/Kick.hpp"

namespace irc
{
void Kick::execute(ClientId actor, const ClientIdentity &identity, const Parameters &params)
{
    Channel *channel = memberChannel(actor, identity, params[0]);
    if (!channel)
        return;
    Parameters fields(1, channel->displayName());
    if (!channel->isOperator(actor))
    {
        numeric(actor, identity, 482, fields, "You're not channel operator");
        return;
    }
    ClientId target;
    ClientIdentity targetIdentity;
    if (!context.identities.clientIdForNickname(params[1], target) ||
        !context.identities.identityOf(target, targetIdentity))
    {
        numeric(actor, identity, 401, Parameters(1, params[1]), "No such nick");
        return;
    }
    if (!channel->containsMember(target))
    {
        fields.insert(fields.begin(), targetIdentity.nickname);
        numeric(actor, identity, 441, fields, "They aren't on that channel");
        return;
    }
}
}
