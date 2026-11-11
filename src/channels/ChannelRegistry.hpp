#ifndef IRC_CHANNELS_CHANNEL_REGISTRY_HPP
#define IRC_CHANNELS_CHANNEL_REGISTRY_HPP

#include <cstddef>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "channels/Channel.hpp"

namespace irc
{
class ChannelRegistry
{
public:
	typedef std::pair<Channel *, bool> CreateResult;

	ChannelRegistry();

	// Names use the same validation and casemapping for every operation.
	Channel *find(const std::string &name);
	const Channel *find(const std::string &name) const;

	// (channel, inserted): existing => (existing, false), invalid => (0, false).
	// Allocation failures propagate, with the registry unchanged.
	CreateResult create(const std::string &name);
	bool eraseEmpty(const std::string &name);
	std::size_t size() const;
	// Detached sorted unique IDs; exclude the actor and non-shared clients.
	std::vector<ClientId> peersOf(ClientId actorId) const;
	// Caller captures peers/identity and queues notifications before cleanup.
	void removeClient(ClientId actorId);

private:
	typedef std::map<std::string, Channel> Channels;

	ChannelRegistry(const ChannelRegistry &other);
	ChannelRegistry &operator=(const ChannelRegistry &other);

	Channels channels_;
};
}

#endif
