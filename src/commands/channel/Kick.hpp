#ifndef IRC_KICK_COMMAND_HPP
#define IRC_KICK_COMMAND_HPP
#include "commands/channel/ChannelCommand.hpp"

namespace irc
{
class Kick : public ChannelCommand
{
public:
    explicit Kick(ChannelContext &ctx) : ChannelCommand(ctx, "KICK", 2) {}
private:
    void execute(ClientId actor, const ClientIdentity &identity, const Parameters &params);
};
}
#endif
