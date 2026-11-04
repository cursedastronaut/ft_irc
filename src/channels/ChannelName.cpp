#include "channels/ChannelName.hpp"
#include "contracts/IrcCaseMapping.hpp"

namespace irc
{
bool isValidChannelName(const std::string &name)
{
    if (name.size() < 2 || name.size() > kMaxChannelNameLength)
        return false;
    if (name[0] != '#' && name[0] != '&')
        return false;
    for (std::string::size_type i = 1; i < name.size(); ++i)
    {
        const unsigned char byte = static_cast<unsigned char>(name[i]);
        if (byte <= 0x20 || byte == 0x7f || byte == ',' || byte == ':')
            return false;
    }
    return true;
}

bool canonicalizeChannelName(const std::string &name, std::string &key)
{
    if (!isValidChannelName(name))
        return false;
    std::string canonical = canonicalizeIrcName(name);
    key.swap(canonical);
    return true;
}
}
