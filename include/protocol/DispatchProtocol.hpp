/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   DispatchProtocol.hpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/18 00:00:00 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/18 00:00:00 by nehuen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef IRC_PROTOCOL_DISPATCH_PROTOCOL_HPP
# define IRC_PROTOCOL_DISPATCH_PROTOCOL_HPP

# include "protocol/Dispatcher.hpp"
# include "protocol/ClientRegistry.hpp"
# include "contracts/Protocol.hpp"

namespace irc
{
	class DispatchProtocol : public ::protocol
	{
		public:
			DispatchProtocol(Dispatcher &dispatcher, ClientRegistry &registry);

			void	connected(ClientId id);
			void	online(ClientId id, const std::string &line);
			void	disconnected(ClientId id, const std::string &reason);

		private:
			Dispatcher		&_dispatcher;
			ClientRegistry	&_registry;
	};
}

#endif
