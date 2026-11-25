#include "Test.hpp"
#include "commands/auth/UserHandler.hpp"

class Output : public transport
{
public:
    std::vector<std::string> lines;
    ClientIdentity returnIdentity(irc::ClientId) const { return ClientIdentity(); }
    void requestDisconnection(irc::ClientId, const std::string &, bool) {}
    enqueueResult enqueueLine(irc::ClientId, const std::string &line)
    {
        lines.push_back(line);
        return Queued;
    }
};

static void userState()
{
    irc::ClientRegistry registry;
    Output output;
    irc::UserHandler user(registry, output);
    registry.create(1);
    std::vector<std::string> p(4);
    p[0] = "alice";
    p[1] = "0";
    p[2] = "*";
    p[3] = "Alice Example";
    irc::ClientRecord record;
    user.handle(1, std::vector<std::string>());
    CHECK(registry.find(1, record) && !record.userSet);
    CHECK(output.lines.back().find(" 461 ") != std::string::npos);
    user.handle(1, p);
    CHECK(registry.find(1, record) && record.realname == "Alice Example");
    CHECK(!record.canRegister() && !registry.isRegistered(1));
    p[0] = "replacement";
    user.handle(1, p);
    CHECK(output.lines.back().find(" 462 ") != std::string::npos);
    CHECK(registry.find(1, record) && record.username == "alice");
    ClientIdentity snapshot;
    snapshot.nickname = "existing";
    CHECK(!registry.identityOf(1, snapshot) && snapshot.nickname == "existing");
    registry.setNickname(1, "Alice");
    registry.setPasswordAccepted(1, true);
    CHECK(registry.find(1, record) && record.canRegister());
    registry.setRegistered(1);
    CHECK(registry.identityOf(1, snapshot));
    CHECK(snapshot.username == "alice" && snapshot.nickname == "Alice" && snapshot.host == "ircserv");
}

int main() { return runTests("USER state and channel identity", userState); }
