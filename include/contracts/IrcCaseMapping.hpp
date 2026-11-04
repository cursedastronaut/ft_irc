#ifndef IRC_CONTRACTS_IRC_CASE_MAPPING_HPP
#define IRC_CONTRACTS_IRC_CASE_MAPPING_HPP

#include <string>

namespace irc
{
// Shared CORE-03 / PROTO-01 proposal: use this for nicknames AND channels.
// RFC1459 equivalences, independent of locale and signed-char behavior.
inline std::string canonicalizeIrcName(const std::string &name)
{
    std::string result(name);
    for (std::string::size_type i = 0; i < result.size(); ++i)
    {
        const unsigned char byte = static_cast<unsigned char>(result[i]);
        if (byte >= 'A' && byte <= 'Z')
            result[i] = static_cast<char>(byte + ('a' - 'A'));
        else if (byte == '[')
            result[i] = '{';
        else if (byte == ']')
            result[i] = '}';
        else if (byte == '\\')
            result[i] = '|';
        else if (byte == '^')
            result[i] = '~';
    }
    return result;
}
}

#endif
