/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   DispatchProtocol.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/18 00:00:00 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/19 17:35:13 by nehuen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "protocol/DispatchProtocol.hpp"
#include "protocol/Parser.hpp"
#include "protocol/Guards.hpp"
#include "protocol/Reply.hpp"
#include <exception>

namespace irc
{
	DispatchProtocol::DispatchProtocol(Dispatcher &dispatcher, ClientRegistry &registry,
					transport &net)
		: _dispatcher(dispatcher), _registry(registry), _transport(net)
	{
	}

	void	DispatchProtocol::connected(ClientId id)
	{
		_registry.create(id);
	}

	void	DispatchProtocol::online(ClientId id, const std::string &line)
	{
		try
		{
			ParsedCommand	out;
			ClientRecord	record;
			bool			known;
			std::string		recipient = "*";

			if (irc::parseCommand(line, out) != ParseSuccess)
				return; //next issue send error to client
			known = _registry.find(id, record);
			if (known && !record.nickname.empty())
				recipient = record.nickname;
			if (out.command == "CAP" && !out.params.empty())
            {
                if (out.params[0] == "LS")
                    _transport.enqueueLine(id, ":ircserv CAP * LS :");
                return;
            }
            if ((!known || !record.registered) && !isPreRegistrationCommand(out.command))
				return; //next issue send error to client
			
			DispatchResult result = _dispatcher.dispatch(id, out);
			if (result == DispatchUnknownCommand)
				sendNumeric(id, 421, recipient, out.command, "Unknown command");
			else if (result == DispatchNeedMoreParams)
				sendNumeric(id, 461, recipient, out.command, "Not enough parameters");
		}
		catch (const std::exception &)
		{
			// a failing command must never escape into the event loop
		}
	}

	void	DispatchProtocol::disconnected(ClientId id, const std::string &reason)
	{
		(void)reason;
		_registry.remove(id);
	}

	void	DispatchProtocol::sendNumeric(ClientId id, int errorCode,
					const std::string &clientPseudo,
					const std::string &command, const std::string &errorMessage)
	{
		std::vector<std::string>	params(1, command);
		std::string					reply;

		if (buildNumericReply(errorCode, clientPseudo, params, errorMessage, reply) == ReplySuccess)
			_transport.enqueueLine(id, reply);
	}
}
