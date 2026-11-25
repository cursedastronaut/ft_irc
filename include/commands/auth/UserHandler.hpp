#ifndef IRC_USER_HANDLER_HPP
#define IRC_USER_HANDLER_HPP
#include "protocol/Dispatcher.hpp"
#include "protocol/ClientRegistry.hpp"

namespace irc
{
class UserHandler : public CommandHandler
{
public:
    UserHandler(ClientRegistry &registry, transport &net) : registry_(registry), net_(net) {}
    void handle(ClientId id, const std::vector<std::string> &params);
private:
    ClientRegistry &registry_;
    transport &net_;
};
}
#endif
