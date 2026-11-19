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

namespace irc
{
	DispatchProtocol::DispatchProtocol(Dispatcher &dispatcher, ClientRegistry &registry)
		: _dispatcher(dispatcher), _registry(registry)
	{
	}

	void	DispatchProtocol::connected(ClientId id)
	{
		_registry.create(id);
	}

	void	DispatchProtocol::online(ClientId id, const std::string &line)
	{
		ParsedCommand	out;

		ClientRecord record;

		if (irc::parseCommand(line, out) != ParseSuccess)
			return; //next issue send error to client
		if ((_registry.find(id, record) == 0 || !record.canRegister()) && !isPreRegistrationCommand(out.command))
			return; //next issue send error to client
		// TODO PROTO-21: canRegister() est un substitut temporaire a "est enregistre")
		if (_dispatcher.dispatch(id, out) == DispatchUnknownCommand)
			return; //next issue send error to client
	}

	void	DispatchProtocol::disconnected(ClientId id, const std::string &reason)
	{
		(void)reason;
		_registry.remove(id);
	}
}
