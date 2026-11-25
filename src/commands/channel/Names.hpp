#ifndef IRC_NAMES_COMMAND_HPP
#define IRC_NAMES_COMMAND_HPP

#include "commands/channel/ChannelCommand.hpp"

namespace irc
{
void sendNames(ChannelContext &ctx, ClientId actor, const ClientIdentity &identity, const Channel &room);

class Names : public ChannelCommand
{
public:
    explicit Names(ChannelContext &ctx) : ChannelCommand(ctx, "NAMES", 1) {}
private:
    void execute(ClientId actor, const ClientIdentity &identity, const Parameters &params);
};
}
#endif
