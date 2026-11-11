#ifndef IRC_TESTS_CHANNEL_CONTRACT_FIXTURES_HPP
#define IRC_TESTS_CHANNEL_CONTRACT_FIXTURES_HPP

#include "contracts/ChannelService.hpp"

irc::IdentityLookup *makeIdentityFixture();
irc::ChannelService *makeChannelFixture();
unsigned int acceptedFixtureCommands(const irc::ChannelService &service);

#endif
