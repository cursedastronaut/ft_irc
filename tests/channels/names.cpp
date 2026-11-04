#include "Test.hpp"
#include "channels/Channel.hpp"
#include "channels/ChannelName.hpp"
#include "contracts/IrcCaseMapping.hpp"

static void equivalentNamesShareAKey()
{
    std::string first;
    std::string second;
    CHECK(irc::canonicalizeChannelName("#MiXeD[\\]^", first));
    CHECK(irc::canonicalizeChannelName("#mixed{|}~", second));
    CHECK(first == "#mixed{|}~");
    CHECK(first == second);
    CHECK(irc::canonicalizeIrcName("Nick[\\]^") == "nick{|}~");

    const irc::Channel channel("#MiXeD[\\]^");
    CHECK(channel.displayName() == "#MiXeD[\\]^");
    CHECK(channel.canonicalName() == first);
    const irc::Channel copy(channel);
    CHECK(copy.canonicalName() == first);
    CHECK(copy.displayName() == channel.displayName());
    CHECK(irc::isValidChannelName("&local"));
    CHECK(irc::canonicalizeIrcName("#local") != irc::canonicalizeIrcName("&local"));
}

static void malformedNamesAreRejected()
{
    const char *invalid[] = {
        "", "#", "&", "room", "room#", "+room", "!room", ":#room",
        " #room", "#bad name", "#bad,name", "#bad:name", "#bad\tname",
        "#bad\rname", "#bad\nname", "#bad\aname", "#room\r\n"
    };
    const std::size_t count = sizeof(invalid) / sizeof(invalid[0]);
    for (std::size_t i = 0; i < count; ++i)
    {
        std::string key("unchanged");
        CHECK(!irc::isValidChannelName(invalid[i]));
        CHECK(!irc::canonicalizeChannelName(invalid[i], key));
        CHECK(key == "unchanged");
        bool rejected = false;
        try
        {
            const irc::Channel channel(invalid[i]);
            (void)channel;
        }
        catch (const std::invalid_argument &)
        {
            rejected = true;
        }
        CHECK(rejected);
    }
}

static void byteLimitsAreExact()
{
    CHECK(irc::isValidChannelName("#a"));
    CHECK(irc::isValidChannelName("&a"));
    CHECK(irc::isValidChannelName("#" + std::string(49, 'a')));
    CHECK(!irc::isValidChannelName("#" + std::string(50, 'a')));
    CHECK(!irc::isValidChannelName("#" + std::string(1000, 'a')));
    std::string multiByte("#");
    for (int i = 0; i < 24; ++i)
        multiByte += "\xc3\xa9";
    multiByte += 'x';
    CHECK(multiByte.size() == 50);
    CHECK(irc::isValidChannelName(multiByte));
    CHECK(!irc::isValidChannelName(multiByte + 'y'));
}

static void allByteValuesAreHandledDeterministically()
{
    for (unsigned int value = 0; value < 256; ++value)
    {
        const char byte = static_cast<char>(value);
        const std::string name = std::string("#a") + byte + 'z';
        const bool forbidden = value <= 0x20 || value == 0x7f
            || value == ',' || value == ':';
        CHECK(irc::isValidChannelName(name) == !forbidden);
        const std::string folded = irc::canonicalizeIrcName(name);
        CHECK(folded.size() == name.size());
        CHECK(irc::canonicalizeIrcName(folded) == folded);
        if (value >= 128)
            CHECK(folded[2] == byte);
    }
    const std::string embeddedNul("#a\0b", 4);
    CHECK(!irc::isValidChannelName(embeddedNul));
}

static void inPlaceCanonicalizationIsSafe()
{
    std::string name("#MiXeD");
    CHECK(irc::canonicalizeChannelName(name, name));
    CHECK(name == "#mixed");
    name = "#invalid name";
    CHECK(!irc::canonicalizeChannelName(name, name));
    CHECK(name == "#invalid name");
}

static void nameTests()
{
    equivalentNamesShareAKey();
    malformedNamesAreRejected();
    byteLimitsAreExact();
    allByteValuesAreHandledDeterministically();
    inPlaceCanonicalizationIsSafe();
}

int main()
{
    return runTests("CHAN-02 channel names", nameTests);
}
