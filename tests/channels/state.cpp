#include "Test.hpp"
#include "channels/ChannelRegistry.hpp"
#include "net/ServerConfig.hpp"

static void defaultsAndSecretsAreIndependent()
{
    const ServerConfig server(6667, "fixture-server-secret");
    irc::ChannelRegistry registry;
    irc::Channel *alpha = registry.create("#alpha").first;
    irc::Channel *beta = registry.create("#beta").first;
    CHECK(alpha != 0 && beta != 0);
    CHECK(alpha->topic.empty());
    CHECK(!alpha->modes.inviteOnly && !alpha->modes.topicRestricted);
    CHECK(!alpha->modes.hasKey && alpha->modes.key.empty());
    CHECK(!alpha->modes.hasLimit && alpha->modes.limit == 0);

    alpha->topic = "a topic with spaces";
    alpha->modes.inviteOnly = true;
    alpha->modes.topicRestricted = true;
    alpha->modes.hasKey = true;
    alpha->modes.key = "fixture-channel-key";
    alpha->modes.hasLimit = true;
    alpha->modes.limit = 4;
    CHECK(server.getPSW() == "fixture-server-secret");
    CHECK(alpha->modes.key != server.getPSW());
    CHECK(beta->topic.empty() && !beta->modes.hasKey);
    irc::Channel copy(*alpha);
    copy.topic.clear();
    copy.modes.key = "changed-copy-key";
    copy.modes.limit = 2;
    CHECK(alpha->topic == "a topic with spaces");
    CHECK(alpha->modes.key == "fixture-channel-key");
    CHECK(alpha->modes.limit == 4);

    // Rejected registry operations cannot reset a valid channel's state.
    CHECK(registry.create("invalid").first == 0);
    CHECK(registry.find("#ALPHA") == alpha);
    CHECK(alpha->topic == "a topic with spaces");
    CHECK(alpha->modes.hasKey && alpha->modes.hasLimit);
    CHECK(registry.eraseEmpty("#alpha"));
    alpha = registry.create("#Alpha").first;
    CHECK(alpha != 0 && alpha->topic.empty());
    CHECK(!alpha->modes.inviteOnly && !alpha->modes.topicRestricted);
    CHECK(!alpha->modes.hasKey && alpha->modes.key.empty());
    CHECK(!alpha->modes.hasLimit && alpha->modes.limit == 0);
    CHECK(server.getPSW() == "fixture-server-secret");
}

int main()
{
    return runTests("CHAN-06 topic and mode value storage", defaultsAndSecretsAreIndependent);
}
