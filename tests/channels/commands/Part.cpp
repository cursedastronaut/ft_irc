#include "Harness.hpp"
#include "commands/channel/Part.hpp"

static void verify()
{
    Harness h;
    irc::Part handler(h.context);
    h.dispatch.registerHandler("PART", &handler);
    irc::Channel &room = h.room();
    h.send(1003, "PART #room :outside");
    CHECK(room.members.size() == 2);
    h.send(1001, "PART #room :back later");
    CHECK(!room.containsMember(1001) && !room.isOperator(1001));
    CHECK(h.wire.lines[1001].back() == ":Alice!user@localhost PART #Room :back later");
    CHECK(h.wire.lines[1002].back() == h.wire.lines[1001].back());
    CHECK(h.wire.lines[1003].size() == 1);
    h.send(1002, "PART #Room");
    CHECK(h.wire.lines[1002].back() == ":Bob!user@localhost PART #Room");
    CHECK(h.registry.size() == 0);
}

int main() { return runTests("Part", verify); }
