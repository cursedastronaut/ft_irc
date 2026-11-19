/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   DispatchProtocol.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/18 00:00:00 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/19 13:50:15 by nehuen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "protocol/DispatchProtocol.hpp"
#include "protocol/Parser.hpp"

namespace irc
{
	DispatchProtocol::DispatchProtocol(Dispatcher &dispatcher)
		: _dispatcher(dispatcher)
	{
	}

	void	DispatchProtocol::connected(ClientId id)
	{
		(void)id;
	}

	void	DispatchProtocol::online(ClientId id, const std::string &line)
	{
		ParsedCommand	out;
		
		if (irc::parseCommand(line, out) != ParseSuccess)
			return; //next issue send error to client
		if (_dispatcher.dispatch(id, out) == DispatchUnknownCommand)
			return; //next issue send error to client
	}

	void	DispatchProtocol::disconnected(ClientId id, const std::string &reason)
	{
		(void)id;
		(void)reason;
	}
}
