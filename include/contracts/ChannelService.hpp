#ifndef IRC_CONTRACTS_CHANNEL_SERVICE_HPP
#define IRC_CONTRACTS_CHANNEL_SERVICE_HPP

#include "contracts/Protocol.hpp"
#include <string>
#include <vector>

namespace irc
{
// Read-only provider to be implemented by Person 2. Failed lookups leave
// output unchanged; successful identity lookup copies a detached snapshot.
class IdentityLookup
{
public:
    virtual ~IdentityLookup() {}
    virtual bool identityOf(ClientId id, ::ClientIdentity &out) const = 0;
    virtual bool clientIdForNickname(const std::string &nickname,
                                     ClientId &out) const = 0;
    virtual bool isRegistered(ClientId id) const = 0;
};

// Person 3 owns the implementation. No socket ownership or wire I/O here.
// Shared contract addition: all three owners review before merge (CORE-03).
class ChannelService
{
public:
    virtual ~ChannelService() {}
    virtual void handle(ClientId actorId, const ParsedCommand &command) = 0;
    // Caller notifies peers while identity is live, then calls this hook.
    virtual void removeClient(ClientId actorId) = 0;
    // Detached, deduplicated IDs, excluding actorId; no nickname identities.
    virtual std::vector<ClientId> peersOf(ClientId actorId) const = 0;
};
}

#endif
