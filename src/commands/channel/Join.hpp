#ifndef IRC_JOIN_COMMAND_HPP
#define IRC_JOIN_COMMAND_HPP
#include "commands/channel/ChannelCommand.hpp"

namespace irc
{
class Join : public ChannelCommand
{
public:
    explicit Join(ChannelContext &ctx) : ChannelCommand(ctx, "JOIN", 1) {}
private:
    void execute(ClientId actor, const ClientIdentity &identity, const Parameters &params);
};
}
#endif
