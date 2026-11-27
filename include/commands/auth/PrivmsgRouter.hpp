/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   PrivmsgRouter.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/26 00:00:00 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/26 00:00:00 by nehuen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef IRC_COMMANDS_AUTH_PRIVMSG_ROUTER_HPP
# define IRC_COMMANDS_AUTH_PRIVMSG_ROUTER_HPP

# include "protocol/ClientRegistry.hpp"
# include "protocol/Dispatcher.hpp"
# include "contracts/Protocol.hpp"

namespace irc
{
	class PrivmsgRouter : public CommandHandler
	{
		public:
			PrivmsgRouter(ClientRegistry &registry, transport &net,
				CommandHandler &channelHandler);

			void	handle(ClientId actor, const std::vector<std::string> &params);

		private:
			ClientRegistry	&_registry;
			transport		&_transport;
			CommandHandler	&_channelHandler;
	};
}

#endif
