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

    CHECK(h.wire.lines[1001].size() == 3 && h.wire.lines[1002].size() == 2);
    CHECK(h.wire.lines[1001][0] == ":Alice!user@localhost JOIN :#Room");
    CHECK(h.wire.lines[1001][1] == ":ircserv 331 Alice #Room :No topic is set");
    CHECK(h.wire.lines[1001][2] == h.wire.lines[1002][0]);
    channel->topic = "weekly news";
    channel->modes.hasKey = true;
    channel->modes.key = "secret";
    h.send(1003, "JOIN #room wrong");
    CHECK(!channel->containsMember(1003) && h.wire.lines[1003].size() == 1);
    CHECK(h.wire.lines[1003][0].find("475 Carol #Room") != std::string::npos);
    CHECK(h.wire.lines[1001].size() == 3);
    channel->modes.hasKey = false;
    h.send(1003, "JOIN #room");
    CHECK(h.wire.lines[1003].back() == ":ircserv 332 Carol #Room :weekly news");
}

int main() { return runTests("Join", verify); }
