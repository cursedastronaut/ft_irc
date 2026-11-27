/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   PrivmsgRouter.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/26 00:00:00 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/26 12:36:40 by nehuen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "commands/auth/PrivmsgRouter.hpp"
#include "protocol/Reply.hpp"
#include "contracts/Protocol.hpp"
namespace irc
{
	PrivmsgRouter::PrivmsgRouter(ClientRegistry &registry, transport &net,
			CommandHandler &channelHandler)
		: _registry(registry), _transport(net), _channelHandler(channelHandler)
	{
	}

	void	PrivmsgRouter::handle(ClientId actor, const std::vector<std::string> &params)
	{
		ClientId target;
		std::string out;
		std::vector<std::string> privmsgParam;
		std::vector<std::string> emptyParam;
		
		if (params.empty() || params[0].empty())
		{
			if (buildNumericReply(411, _registry.recipientFor(actor), privmsgParam, "No recipient given (PRIVMSG)", out) == ReplySuccess)
				_transport.enqueueLine(actor, out);
			return;
		}
		privmsgParam.push_back(params[0]);
		if (params[0][0] == '#' || params[0][0] == '&')
		{
			_channelHandler.handle(actor, params);
			return;
		}
		else if (params.size() < 2 || params[1] == "")
		{
			if (buildNumericReply(412, _registry.recipientFor(actor), emptyParam, "No text to send", out) == ReplySuccess)
				_transport.enqueueLine(actor, out);
			return;
		}
		else if (!_registry.clientIdForNickname(params[0], target))
		{
			if (buildNumericReply(401, _registry.recipientFor(actor), privmsgParam, "No such nick/channel", out) == ReplySuccess)
				_transport.enqueueLine(actor, out);
			return;
		}
		ClientIdentity identity;
		if (_registry.identityOf(actor, identity) && buildEventLine(identity,
			 "PRIVMSG", std::vector<std::string>(1, params[0]), params[1], out) == ReplySuccess)
			_transport.enqueueLine(target, out);
	}
}
