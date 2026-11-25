#ifndef IRC_CHANNEL_MESSAGE_HPP
#define IRC_CHANNEL_MESSAGE_HPP
#include "commands/channel/ChannelCommand.hpp"

namespace irc
{
class ChannelMessage : public ChannelCommand
{
public:
    explicit ChannelMessage(ChannelContext &ctx) : ChannelCommand(ctx, "PRIVMSG", 2) {}
private:
    void execute(ClientId actor, const ClientIdentity &identity, const Parameters &params);
};
}
#endif
