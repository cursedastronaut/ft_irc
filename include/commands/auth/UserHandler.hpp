/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   UserHandler.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/24 14:04:42 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/24 22:49:30 by nehuen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef IRC_COMMANDS_AUTH_USER_HANDLER_HPP
# define IRC_COMMANDS_AUTH_USER_HANDLER_HPP

# include "protocol/ClientRegistry.hpp"
# include "contracts/Protocol.hpp"
# include "protocol/Dispatcher.hpp" 

namespace irc
{
	class UserHandler : public CommandHandler
	{
		public :
			UserHandler(ClientRegistry &registry, transport &net);
			
			void	handle(ClientId actor, const std::vector<std::string> &params);
			
		private :
			ClientRegistry	&_registry;
			transport		&_transport;
	};
}

#endif
