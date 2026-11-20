#include "Harness.hpp"
#include "commands/channel/Invite.hpp"

static void verify()
{
    Harness h;
    irc::Invite handler(h.context);
    h.dispatch.registerHandler("INVITE", &handler);
    irc::Channel &room = h.room();
    h.send(1001, "INVITE Unknown #room");
    CHECK(h.wire.lines[1001].back().find("401 Alice Unknown") != std::string::npos);
    h.send(1001, "INVITE bOB #room");
    CHECK(h.wire.lines[1001].back().find("443 Alice bOB #Room") != std::string::npos);
    h.send(1003, "INVITE Bob #room");
    CHECK(h.wire.lines[1003].back().find("442 Carol") != std::string::npos);
    CHECK(room.invitations.empty());
    h.send(1002, "INVITE Carol #room");
    CHECK(room.hasInvitation(1003));
}

int main() { return runTests("Invite", verify); }
