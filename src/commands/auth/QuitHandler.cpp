/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   QuitHandler.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/26 00:00:00 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/26 17:56:14 by nehuen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "commands/auth/QuitHandler.hpp"
#include "protocol/Reply.hpp"

namespace irc
{
	QuitHandler::QuitHandler(ClientRegistry &registry, transport &net,
			ChannelService &channels)
		: _registry(registry), _transport(net), _channels(channels)
	{
	}

	void	QuitHandler::handle(ClientId actor, const std::vector<std::string> &params)
	{
		ClientRecord record;
		if (!_registry.find(actor, record))
			return ;
		std::string reason = (params.empty() || params[0].empty()) 
			? "Client Quit" : params[0];
		std::vector<ClientId> peers = _channels.peersOf(actor);
		
		ClientIdentity quiter;
		if (_registry.identityOf(actor, quiter))
		{
			std::string line;
			if (buildEventLine(quiter, 
					"QUIT", std::vector<std::string>(), reason, line) == ReplySuccess)
			{
				//envoi le message quit a tous les peers
				for (std::vector<ClientId>::const_iterator it = peers.begin();
					 it != peers.end(); ++it)
					 _transport.enqueueLine(*it, line);
			}
		}
		_transport.requestDisconnection(actor, reason, true);

	}
}
