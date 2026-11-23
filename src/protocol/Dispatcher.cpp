/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Dispatcher.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/16 00:00:00 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/17 11:51:22 by nehuen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "protocol/Dispatcher.hpp"

namespace irc
{
	void	Dispatcher::registerHandler(const std::string &command,
					CommandHandler *handler, std::size_t minParams)
	{
		_handlers[command] = handler;
		_minParams[command] = minParams;
	}

	DispatchResult	Dispatcher::dispatch(ClientId actor,
					const ParsedCommand &command) const
	{
		std::map<std::string, CommandHandler *>::const_iterator it = _handlers.find(command.command);
		if (it == _handlers.end())
			return DispatchUnknownCommand;
		if (command.params.size() < _minParams.find(command.command)->second)
			return DispatchNeedMoreParams;
		it->second->handle(actor, command.params);
		return DispatchHandled;
	}
}
