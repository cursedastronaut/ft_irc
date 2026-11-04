#include "channels/ChannelRegistry.hpp"
#include "channels/ChannelName.hpp"

namespace irc
{
ChannelRegistry::ChannelRegistry() : channels_()
{
}

Channel *ChannelRegistry::find(const std::string &name)
{
    std::string key;
    if (!canonicalizeChannelName(name, key))
        return 0;
    Channels::iterator found = channels_.find(key);
    if (found == channels_.end())
        return 0;
    return &found->second;
}

const Channel *ChannelRegistry::find(const std::string &name) const
{
    std::string key;
    if (!canonicalizeChannelName(name, key))
        return 0;
    Channels::const_iterator found = channels_.find(key);
    if (found == channels_.end())
        return 0;
    return &found->second;
}

ChannelRegistry::CreateResult ChannelRegistry::create(const std::string &name)
{
    std::string key;
    if (!canonicalizeChannelName(name, key))
        return CreateResult(0, false);
    Channels::iterator found = channels_.find(key);
    if (found != channels_.end())
        return CreateResult(&found->second, false);

    const std::pair<Channels::iterator, bool> result = channels_.insert(
        Channels::value_type(key, Channel(name)));
    return CreateResult(&result.first->second, result.second);
}

bool ChannelRegistry::eraseEmpty(const std::string &name)
{
    std::string key;
    if (!canonicalizeChannelName(name, key))
        return false;
    Channels::iterator found = channels_.find(key);
    if (found == channels_.end() || !found->second.members.empty())
        return false;
    channels_.erase(found);
    return true;
}

std::size_t ChannelRegistry::size() const
{
    return channels_.size();
}
}
