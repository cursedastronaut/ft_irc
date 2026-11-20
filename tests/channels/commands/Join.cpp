#include "Harness.hpp"
#include "commands/channel/Join.hpp"

static void verify()
{
    Harness h;
    irc::Join handler(h.context);
    h.dispatch.registerHandler("JOIN", &handler);
    h.send(9999, "JOIN #Room");
    CHECK(h.registry.size() == 0);
    h.send(1001, "JOIN #Room");
    irc::Channel *channel = h.registry.find("#room");
    CHECK(channel && channel->isOperator(1001));
    h.send(1002, "JOIN #ROOM");
    h.send(1002, "JOIN #Room");
    CHECK(channel->members.size() == 2 && !channel->isOperator(1002));
    channel->revokeOperator(1001);
    h.send(1001, "JOIN #room");
    CHECK(!channel->isOperator(1001));
}

int main() { return runTests("Join", verify); }
