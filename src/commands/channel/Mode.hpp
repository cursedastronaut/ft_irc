#ifndef IRC_MODE_COMMAND_HPP
#define IRC_MODE_COMMAND_HPP
#include "commands/channel/ChannelCommand.hpp"

namespace irc
{
struct ModeStep
{
    bool adding;
    char flag;
    std::string argument;
    int error;
};
std::vector<ModeStep> planModeChanges(const Parameters &params);

class Mode : public ChannelCommand
{
public:
    explicit Mode(ChannelContext &ctx) : ChannelCommand(ctx, "MODE", 1) {}
private:
    void apply(Channel &room, ClientId actor, const ClientIdentity &identity, const ModeStep &step);
    void execute(ClientId actor, const ClientIdentity &identity, const Parameters &params);
};
}
#endif
