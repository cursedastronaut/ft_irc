#include "Test.hpp"
#include "channels/ChannelRegistry.hpp"

static void peersAreUniqueDetachedAndLimitedToSharedChannels()
{
    irc::ChannelRegistry registry;
    irc::Channel *alpha = registry.create("#alpha").first;
    irc::Channel *beta = registry.create("#beta").first;
    irc::Channel *other = registry.create("#other").first;
    CHECK(alpha && beta && other);
    alpha->addMember(1001);
    alpha->addMember(1002);
    beta->addMember(1001);
    beta->addMember(1002);
    beta->addMember(1003);
    other->addMember(1004);
    alpha->addInvitation(1005); // An invitation is not shared membership.
    const irc::ChannelRegistry &view = registry;
    std::vector<irc::ClientId> peers = view.peersOf(1001);
    CHECK(peers.size() == 2 && peers[0] == 1002 && peers[1] == 1003);
    CHECK(view.peersOf(9999).empty());
    CHECK(view.peersOf(1004).empty());
    CHECK(view.peersOf(1005).empty());
    beta->removeMember(1003);
    CHECK(peers.size() == 2 && peers[1] == 1003); // Value survives mutation.
    peers.clear();
    CHECK(view.peersOf(1001).size() == 1);
    alpha->removeMember(1001);
    beta->removeMember(1001);
    CHECK(view.peersOf(1001).empty());
}

int main()
{
    return runTests("CHAN-09 detached unique peers", peersAreUniqueDetachedAndLimitedToSharedChannels);
}
