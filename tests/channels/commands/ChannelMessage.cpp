#include "Harness.hpp"
#include "commands/channel/ChannelMessage.hpp"

static void verify()
{
    Harness h;
    irc::ChannelMessage handler(h.context);
    h.dispatch.registerHandler("PRIVMSG", &handler);
    irc::Channel &room = h.room();
    h.send(1001, "PRIVMSG #room :hello there");
    CHECK(h.wire.lines.size() == 1 && h.wire.lines[1002].size() == 1);
    CHECK(h.wire.lines[1002][0] == ":Alice!user@localhost PRIVMSG #Room :hello there");
    h.send(1003, "PRIVMSG #room :outside");
    CHECK(h.wire.lines[1003].back().find("404 Carol #Room") != std::string::npos);
    room.removeMember(1001);
    h.send(1001, "PRIVMSG #room :after departure");
    CHECK(h.wire.lines[1001].back().find("404 Alice #Room") != std::string::npos);
    CHECK(h.wire.lines[1002].size() == 1);
}
int main() { return runTests("Channel PRIVMSG", verify); }
