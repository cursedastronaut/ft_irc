#include "Harness.hpp"
#include "commands/channel/Names.hpp"
#include <sstream>

class ManyUsers : public ChannelUsers
{
public:
    bool identityOf(irc::ClientId id, ClientIdentity &out) const
    {
        std::ostringstream nick;
        nick << "Guest" << id;
        out.nickname = nick.str();
        return true;
    }
    bool isRegistered(irc::ClientId) const { return true; }
};

static void verify()
{
    ManyUsers users;
    Harness h;
    irc::ChannelContext ctx(h.registry, users, h.wire);
    irc::Names names(ctx);
    h.dispatch.registerHandler("NAMES", &names);
    irc::Channel &room = *h.registry.create("#room").first;
    std::set<std::string> expected;
    for (size_t id = 1001; id <= 1150; ++id)
    {
        room.addMember(id);
        ClientIdentity user;
        users.identityOf(id, user);
        expected.insert((id == 1001 ? "@" : "") + user.nickname);
    }
    room.grantOperator(1001);
    h.send(1001, "NAMES #room");
    const irc::Parameters &lines = h.wire.lines[1001];
    CHECK(lines.size() > 2 && lines.back().find(" 366 Guest1001 #room ") != std::string::npos);
    for (size_t i = 0; i + 1 < lines.size(); ++i)
    {
        const std::string header = ":ircserv 353 Guest1001 = #room :";
        CHECK(lines[i].size() <= 510 && lines[i].find(header) == 0);
        std::istringstream tokens(lines[i].substr(header.size()));
        std::string nick;
        while (tokens >> nick)
            CHECK(expected.erase(nick) == 1);
    }
    CHECK(expected.empty());
    h.send(1999, "NAMES #room");
    CHECK(h.wire.lines[1999].size() == 1 && h.wire.lines[1999][0].find(" 442 ") != std::string::npos);
}
int main() { return runTests("NAMES split and membership", verify); }
