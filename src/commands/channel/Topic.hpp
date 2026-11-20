#ifndef IRC_TOPIC_COMMAND_HPP
#define IRC_TOPIC_COMMAND_HPP
#include "commands/channel/ChannelCommand.hpp"

namespace irc
{
class Topic : public ChannelCommand
{
public:
    explicit Topic(ChannelContext &ctx) : ChannelCommand(ctx, "TOPIC", 1) {}
private:
    void execute(ClientId actor, const ClientIdentity &identity, const Parameters &params);
};
}
#endif
