#ifndef IRC_INVITE_COMMAND_HPP
#define IRC_INVITE_COMMAND_HPP
#include "commands/channel/ChannelCommand.hpp"

namespace irc
{
class Invite : public ChannelCommand
{
public:
    explicit Invite(ChannelContext &ctx) : ChannelCommand(ctx, "INVITE", 2) {}
private:
    void execute(ClientId actor, const ClientIdentity &identity, const Parameters &params);
};
}
#endif
