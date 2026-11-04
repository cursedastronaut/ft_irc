#include "channels/Channel.hpp"
#include "channels/ChannelName.hpp"

#include <stdexcept>

namespace
{
std::string validatedCanonicalName(const std::string &name)
{
	std::string canonical;
	if (!irc::canonicalizeChannelName(name, canonical))
		throw std::invalid_argument("Invalid channel name");
	return canonical;
}
}

namespace irc
{
ChannelModes::ChannelModes()
	: inviteOnly(false), topicRestricted(false), hasKey(false), key(),
	  hasLimit(false), limit(0)
{
}

Channel::Channel(const std::string &name)
	: members(), operators(), invitations(), topic(), modes(),
	  canonicalName_(validatedCanonicalName(name)), displayName_(name)
{
}

Channel::Channel(const Channel &other)
	: members(other.members), operators(other.operators),
	  invitations(other.invitations), topic(other.topic), modes(other.modes),
	  canonicalName_(other.canonicalName_), displayName_(other.displayName_)
{
}

const std::string &Channel::canonicalName() const
{
	return canonicalName_;
}

const std::string &Channel::displayName() const
{
	return displayName_;
}
}
