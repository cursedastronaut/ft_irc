#ifndef IRC_CHANNELS_CHANNEL_NAME_HPP
#define IRC_CHANNELS_CHANNEL_NAME_HPP

#include <cstddef>
#include <string>

namespace irc
{
const std::size_t kMaxChannelNameLength = 50;

// A single # or & channel name, 2..50 bytes. No lists or server masks.
bool isValidChannelName(const std::string &name);

// Invalid input leaves key unchanged. Allocation failures propagate.
bool canonicalizeChannelName(const std::string &name, std::string &key);
}

#endif
