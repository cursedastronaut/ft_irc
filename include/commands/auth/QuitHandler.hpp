/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   QuitHandler.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/26 00:00:00 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/26 00:00:00 by nehuen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef IRC_COMMANDS_AUTH_QUIT_HANDLER_HPP
# define IRC_COMMANDS_AUTH_QUIT_HANDLER_HPP

# include "protocol/ClientRegistry.hpp"
# include "protocol/Dispatcher.hpp"
# include "contracts/Protocol.hpp"
# include "contracts/ChannelService.hpp"

namespace irc
{
	class QuitHandler : public CommandHandler
	{
		public:
			QuitHandler(ClientRegistry &registry, transport &net,
				ChannelService &channels);

			void	handle(ClientId actor, const std::vector<std::string> &params);

		private:
			ClientRegistry	&_registry;
			transport		&_transport;
			ChannelService	&_channels;
	};
}

#endif
