/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   PassHandler.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/22 00:00:00 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/25 15:51:08 by nehuen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "commands/auth/PassHandler.hpp"
#include "protocol/Reply.hpp"
#include "commands/auth/Welcome.hpp"

namespace irc
{
	PassHandler::PassHandler(ClientRegistry &registry,
			const PasswordSource &source, transport &net)
		: _registry(registry), _source(source), _transport(net) {
	}

	void	PassHandler::handle(ClientId actor, const std::vector<std::string> &params)
	{
		ClientRecord				record;
		std::vector<std::string>	noParams;
		std::string					line;

		if (!_registry.find(actor, record))
			return ;
		if (record.registered)
		{
			if (buildNumericReply(462, _registry.recipientFor(actor), noParams,
					"Unauthorized command (already registered)", line) == ReplySuccess)
				_transport.enqueueLine(actor, line);
			return ;
		}
		if (_registry.setPasswordAccepted(actor, params[0] == _source.getPassword()))
			sendWelcome(actor, _registry, _transport);
	}
}
