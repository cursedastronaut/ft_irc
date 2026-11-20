#ifndef IRC_PART_COMMAND_HPP
#define IRC_PART_COMMAND_HPP
#include "commands/channel/ChannelCommand.hpp"

namespace irc
{
class Part : public ChannelCommand
{
public:
    explicit Part(ChannelContext &ctx) : ChannelCommand(ctx, "PART", 1) {}
private:
    void execute(ClientId actor, const ClientIdentity &identity, const Parameters &params);
};
}
#endif
