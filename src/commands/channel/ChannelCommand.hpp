#ifndef IRC_CHANNEL_COMMAND_HPP
#define IRC_CHANNEL_COMMAND_HPP

#include "channels/ChannelRegistry.hpp"
#include "contracts/ChannelService.hpp"
#include "protocol/Dispatcher.hpp"
#include "protocol/Reply.hpp"

namespace irc
{
typedef std::vector<std::string> Parameters;

struct ChannelContext
{
    ChannelContext(ChannelRegistry &channels, const IdentityLookup &users, transport &wire)
        : registry(channels), identities(users), output(wire) {}
    ChannelRegistry &registry;
    const IdentityLookup &identities;
    transport &output;
};

class ChannelCommand : public CommandHandler
{
public:
    void handle(ClientId actor, const Parameters &params);
protected:
    ChannelCommand(ChannelContext &context, const std::string &name, size_t minimum);
    virtual void execute(ClientId actor, const ClientIdentity &identity,
                         const Parameters &params) = 0;
    void numeric(ClientId actor, const ClientIdentity &identity, int code,
                 const Parameters &fields, const std::string &text);
    Channel *memberChannel(ClientId actor, const ClientIdentity &identity,
                           const std::string &name);
    void broadcast(const Channel &channel, const std::string &line);
    ChannelContext &context;
private:
    const std::string name_;
    const size_t minimum_;
};
}
#endif
