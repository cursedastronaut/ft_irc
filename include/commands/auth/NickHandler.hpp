/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   NickHandler.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/23 18:13:12 by nd-angel          #+#    #+#             */
/*   Updated: 2026/11/27 22:51:38 by nehuen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef IRC_COMMANDS_AUTH_NICK_HANDLER_HPP
# define IRC_COMMANDS_AUTH_NICK_HANDLER_HPP

# include <string>
# include <vector>
# include "protocol/Dispatcher.hpp"
# include "protocol/ClientRegistry.hpp"
# include "contracts/Protocol.hpp"

namespace irc
{
	bool	isValidNickname(const std::string &nick);
	
	class NickHandler : public CommandHandler
	{
		public:
			NickHandler(ClientRegistry &registry, transport &net, 
				ChannelService &channels);

			void	handle(ClientId actor, const std::vector<std::string> &params);

		private:
			void	reply(ClientId actor, int numeric,
						const std::vector<std::string> &params,
						const std::string &text);

			ClientRegistry		&_registry;
			transport					&_transport;
			ChannelService		&_channels;
	};
}

#endif
