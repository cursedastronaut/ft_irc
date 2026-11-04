#ifndef IRC_CHANNELS_CHANNEL_HPP
#define IRC_CHANNELS_CHANNEL_HPP

#include <cstddef>
#include <set>
#include <string>

#include "contracts/ClientId.hpp"

namespace irc
{
struct ChannelModes
{
    ChannelModes();

    bool inviteOnly;
    bool topicRestricted;
    bool hasKey;
    std::string key;
    bool hasLimit;
    std::size_t limit;
};

class Channel
{
public:
    typedef std::set<ClientId> ClientIds;

    // Validates the name, derives its canonical key and retains its spelling.
    // Throws std::invalid_argument for malformed names.
    explicit Channel(const std::string &name);
    Channel(const Channel &other);

    const std::string &canonicalName() const;
    const std::string &displayName() const;

    // Value storage only. Channel command/service code owns validation and
    // policy (including keeping operators a subset of members).
    ClientIds members;
    ClientIds operators;
    ClientIds invitations;
    std::string topic;
    ChannelModes modes;

private:
    // A channel's identity cannot change while stored in a registry.
    Channel &operator=(const Channel &other);

    const std::string canonicalName_;
    const std::string displayName_;
};
}

#endif
