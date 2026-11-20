#include "Harness.hpp"
#include "commands/channel/Mode.hpp"

static void verify()
{
    Harness h;
    irc::Mode handler(h.context);
    h.dispatch.registerHandler("MODE", &handler);
    irc::Channel &room = h.room();
    irc::ParsedCommand command;
    CHECK(irc::parseCommand("MODE #room +xk-o+l key Bob 3", command) == irc::ParseSuccess);
    std::vector<irc::ModeStep> plan = irc::planModeChanges(command.params);
    CHECK(plan.size() == 4 && plan[0].error == 472);
    CHECK(plan[1].adding && plan[1].argument == "key");
    CHECK(!plan[2].adding && plan[2].argument == "Bob");
    CHECK(plan[3].adding && plan[3].argument == "3");
    h.send(1001, "MODE #room +xo+l");
    CHECK(h.wire.lines[1001].size() == 2);
    CHECK(h.wire.lines[1001][0].find("472 Alice x") != std::string::npos);
    CHECK(h.wire.lines[1001][1].find("461 Alice MODE") != std::string::npos);
    CHECK(!room.isOperator(1002));
}

int main() { return runTests("Mode", verify); }
