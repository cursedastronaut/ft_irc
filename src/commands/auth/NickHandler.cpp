/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   NickHandler.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nd-angel <nd-angel@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/23 18:16:20 by nd-angel          #+#    #+#             */
/*   Updated: 2026/11/23 21:26:53 by nd-angel         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "commands/auth/NickHandler.hpp"
#include "protocol/Reply.hpp"
#include <cctype>

namespace irc
{
	static bool	isSpecial(char c)
	{
		static const std::string	specials("[]\\`_^{|}");

		return specials.find(c) != std::string::npos;
	}

	bool	isValidNickname(const std::string &nick)
	{
		if (nick.empty() || nick.size() > 9)
			return false;
		if (!std::isalpha(static_cast<unsigned char>(nick[0]))
			&& !isSpecial(nick[0]))
			return false;
		for (std::string::size_type i = 1; i < nick.size(); ++i)
		{
			unsigned char	c = static_cast<unsigned char>(nick[i]);

			if (!std::isalnum(c) && !isSpecial(nick[i]) && nick[i] != '-')
				return false;
		}
		return true;
	}

	NickHandler::NickHandler(ClientRegistry &registry, transport &net)
		: _registry(registry), _transport(net) {
	}

	void	NickHandler::reply(ClientId actor, int numeric,
				const std::vector<std::string> &params, const std::string &text)
	{
		std::string	line;

		if (buildNumericReply(numeric, _registry.recipientFor(actor), params,
				text, line) == ReplySuccess)
			_transport.enqueueLine(actor, line);
	}

	void	NickHandler::handle(ClientId actor, const std::vector<std::string> &params)
	{
		ClientRecord				record;
		ClientId					owner;
		std::vector<std::string>	nickParam;

		if (!_registry.find(actor, record))
			return ;
		if (params.empty() || params[0].empty())
			return reply(actor, 431, nickParam, "No nickname given");
		nickParam.push_back(params[0]);
		if (!isValidNickname(params[0]))
			return reply(actor, 432, nickParam, "Erroneous nickname");
		if (_registry.clientIdForNickname(params[0], owner) && owner != actor)
			return reply(actor, 433, nickParam, "Nickname is already in use");
		_registry.setNickname(actor, params[0]);
	}
}
