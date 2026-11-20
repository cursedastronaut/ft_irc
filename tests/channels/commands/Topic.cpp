#include "Harness.hpp"
#include "commands/channel/Topic.hpp"

static void verify()
{
    Harness h;
    irc::Topic handler(h.context);
    h.dispatch.registerHandler("TOPIC", &handler);
    irc::Channel &room = h.room();
    h.send(1002, "TOPIC #room");
    CHECK(h.wire.lines[1002].back() == ":ircserv 331 Bob #Room :No topic is set");
    room.topic = "private news";
    h.send(1003, "TOPIC #room");
    CHECK(h.wire.lines[1003].back() == ":ircserv 442 Carol #Room :You're not on that channel");
    h.send(1001, "TOPIC #missing");
    CHECK(h.wire.lines[1001].back().find("403 Alice #missing") != std::string::npos);
    h.send(1002, "TOPIC #room");
    CHECK(h.wire.lines[1002].back() == ":ircserv 332 Bob #Room :private news");
    CHECK(room.topic == "private news");

    h.send(1002, "TOPIC #room :new topic");
    CHECK(room.topic == "new topic");
    CHECK(h.wire.lines[1001].back() == ":Bob!user@localhost TOPIC #Room :new topic");
    CHECK(h.wire.lines[1002].back() == h.wire.lines[1001].back());
    CHECK(h.wire.lines[1003].size() == 1);
    room.modes.topicRestricted = true;
    h.send(1002, "TOPIC #room :denied");
    CHECK(room.topic == "new topic");
}

int main() { return runTests("Topic", verify); }
