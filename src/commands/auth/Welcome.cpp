/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Welcome.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/25 14:16:05 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/25 15:42:33 by nehuen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "commands/auth/Welcome.hpp"
#include "protocol/Reply.hpp"

namespace irc
{
	void	sendWelcome(ClientId id, ClientRegistry &registry, transport &net)
	{
		ClientRecord record;
		ClientIdentity identity;
		std::vector<std::string> noParams;
		if (!registry.find(id, record))
			return ;
		identity = net.returnIdentity(id);
	

		const std::string text = "Welcome to the Internet Relay Network " 
		+ record.nickname + "!" + record.username + 
		"@" + identity.host;
	
		std::string line;
		if (buildNumericReply(1, registry.recipientFor(id), noParams, text, line) == ReplySuccess)
			net.enqueueLine(id, line);
	}
}
