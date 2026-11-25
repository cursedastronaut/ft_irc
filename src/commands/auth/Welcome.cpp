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
		ClientIdentity identity;
		std::vector<std::string> noParams;

		// registry.identityOf() provides a server-owned host mask,
		// independent of net.returnIdentity() (still a stub on the
		// transport side as of PROTO-20/21/22).
		if (!registry.identityOf(id, identity))
			return ;

		const std::string text = "Welcome to the Internet Relay Network "
			+ identity.nickname + "!" + identity.username + "@" + identity.host;

		std::string line;
		if (buildNumericReply(1, registry.recipientFor(id), noParams, text, line) == ReplySuccess)
			net.enqueueLine(id, line);

		// RPL_ENDOFMOTD substitute (no MOTD file): lets irssi leave the
		// connecting state and run its queued commands (PROTO-33 note).
		std::string end;
		if (buildNumericReply(422, registry.recipientFor(id), noParams,
				"MOTD File is missing", end) == ReplySuccess)
			net.enqueueLine(id, end);
	}
}
