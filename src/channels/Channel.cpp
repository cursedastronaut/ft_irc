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

bool Channel::containsMember(ClientId id) const
{
	return members.find(id) != members.end();
}

bool Channel::addMember(ClientId id)
{
	return members.insert(id).second;
}

bool Channel::removeMember(ClientId id)
{
	operators.erase(id);
	return members.erase(id) != 0;
}

bool Channel::hasInvitation(ClientId id) const
{
	return invitations.find(id) != invitations.end();
}

bool Channel::addInvitation(ClientId id)
{
	return invitations.insert(id).second;
}

bool Channel::consumeInvitation(ClientId id)
{
	if (!containsMember(id))
		return false;
	return invitations.erase(id) != 0;
}

bool Channel::revokeInvitation(ClientId id)
{
	return invitations.erase(id) != 0;
}

bool Channel::isOperator(ClientId id) const
{
	return containsMember(id) && operators.find(id) != operators.end();
}

bool Channel::grantOperator(ClientId id)
{
	if (!containsMember(id))
		return false;
	return operators.insert(id).second;
}

bool Channel::revokeOperator(ClientId id)
{
	if (!containsMember(id))
		return false;
	return operators.erase(id) != 0;
}
}
