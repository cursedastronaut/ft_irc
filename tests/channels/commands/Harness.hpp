#ifndef IRC_CHANNEL_TEST_HARNESS_HPP
#define IRC_CHANNEL_TEST_HARNESS_HPP

#include "../Test.hpp"
#include "contracts/IrcCaseMapping.hpp"
#include "commands/channel/ChannelCommand.hpp"
#include "protocol/Parser.hpp"
class ChannelUsers : public irc::IdentityLookup
{
public:
    bool identityOf(irc::ClientId id, ClientIdentity &out) const
    {
        if (!isRegistered(id))
            return false;
        static const char *names[] = {"Alice", "Bob", "Carol"};
        out.nickname = names[id - 1001];
        out.username = "user";
        out.host = "localhost";
        return true;
    }
    bool clientIdForNickname(const std::string &nick, irc::ClientId &out) const
    {
        for (irc::ClientId id = 1001; id <= 1003; ++id)
        {
            ClientIdentity user;
            identityOf(id, user);
            if (irc::canonicalizeIrcName(user.nickname) == irc::canonicalizeIrcName(nick))
            {
                out = id;
                return true;
            }
        }
        return false;
    }
    bool isRegistered(irc::ClientId id) const { return id >= 1001 && id <= 1003; }
};

class RecordingTransport : public transport
{
public:
    std::map<irc::ClientId, irc::Parameters> lines;
    ClientIdentity returnIdentity(irc::ClientId) const { return ClientIdentity(); }
    void requestDisconnection(irc::ClientId, const std::string &, bool) {}
    enqueueResult enqueueLine(irc::ClientId id, const std::string &line)
    {
        CHECK(line.find_first_of("\r\n") == std::string::npos);
        lines[id].push_back(line);
        return Queued;
    }
};

struct Harness
{
    ChannelUsers users;
    irc::ChannelRegistry registry;
    RecordingTransport wire;
    irc::ChannelContext context;
    irc::Dispatcher dispatch;
    Harness() : context(registry, users, wire) {}
    void send(irc::ClientId actor, const std::string &line)
    {
        irc::ParsedCommand command;
        CHECK(irc::parseCommand(line, command) == irc::ParseSuccess);
        CHECK(dispatch.dispatch(actor, command) == irc::DispatchHandled);
    }
    irc::Channel &room()
    {
        irc::Channel &channel = *registry.create("#Room").first;
        channel.addMember(1001);
        channel.grantOperator(1001);
        channel.addMember(1002);
        return channel;
    }
};
#endif
