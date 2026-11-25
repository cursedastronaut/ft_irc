#ifndef IRC_WHO_COMMAND_HPP
#define IRC_WHO_COMMAND_HPP

#include "commands/channel/ChannelCommand.hpp"

namespace irc
{
class Who : public ChannelCommand
{
public:
    explicit Who(ChannelContext &ctx) : ChannelCommand(ctx, "WHO", 1) {}
private:
    void execute(ClientId actor, const ClientIdentity &identity, const Parameters &params);
};
}
#endif
