/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   UserHandler.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/24 14:09:17 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/25 15:52:04 by nehuen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "commands/auth/UserHandler.hpp"
#include "protocol/Dispatcher.hpp"
#include "protocol/Reply.hpp"
#include "commands/auth/Welcome.hpp"

namespace irc
{
	UserHandler::UserHandler(ClientRegistry &registry,
		 transport &net) : _registry(registry), _transport(net)
	{		
	}

	void	UserHandler::handle(ClientId id, const std::vector<std::string> &params)
	{
		ClientRecord							record;
		std::vector<std::string>	noParams;
		std::string								line;
		
		if (!_registry.find(id, record))
			return ;
		if (record.registered)
		{
			if (buildNumericReply(462, _registry.recipientFor(id), noParams,
					"Unauthorized command (already registered)", line) == ReplySuccess)
				_transport.enqueueLine(id, line);
			return ;
		}
		if (params[0].empty() || params[1].empty() || params[2].empty() || params[3].empty()
			|| params[0].find_first_of(" !@:\r\n\t") != std::string::npos
			|| params[0].find('\0') != std::string::npos
			|| params[0].size() > 64)
		{
			if (buildNumericReply(461, _registry.recipientFor(id), std::vector<std::string>(1, "USER"),
					"Invalid USER parameters", line) == ReplySuccess)
				_transport.enqueueLine(id, line);
			return ;
		}
		if (_registry.setUser(id, params[0], params[3]))
			sendWelcome(id, _registry, _transport);
	}
}
