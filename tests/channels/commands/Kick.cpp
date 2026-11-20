#include "Harness.hpp"
#include "commands/channel/Kick.hpp"

static void verify()
{
    Harness h;
    irc::Kick handler(h.context);
    h.dispatch.registerHandler("KICK", &handler);
    irc::Channel &room = h.room();
    h.send(1002, "KICK #room Alice");
    CHECK(h.wire.lines[1002].back().find("482 Bob") != std::string::npos);
    h.send(1003, "KICK #room Bob");
    CHECK(h.wire.lines[1003].back().find("442 Carol") != std::string::npos);
    h.send(1001, "KICK #room unknown");
    CHECK(h.wire.lines[1001].back().find("401 Alice") != std::string::npos);
    h.send(1001, "KICK #room Carol");
    CHECK(h.wire.lines[1001].back().find("441 Alice Carol #Room") != std::string::npos);
    CHECK(room.members.size() == 2 && room.isOperator(1001));
}

int main() { return runTests("Kick", verify); }
