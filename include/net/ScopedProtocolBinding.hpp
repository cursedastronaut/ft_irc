#ifndef SCOPEDPROTOCOLBINDING_HPP
# define SCOPEDPROTOCOLBINDING_HPP

# include "net/ServerConfig.hpp"

namespace irc
{
class ScopedProtocolBinding {
	private:
		irc::ServerConfig	&_serv;

		ScopedProtocolBinding(const ScopedProtocolBinding &);
		ScopedProtocolBinding	&operator=(const ScopedProtocolBinding &);

	public:
		ScopedProtocolBinding(irc::ServerConfig &serv, protocol *proto);
		~ScopedProtocolBinding();
};

}

#endif
