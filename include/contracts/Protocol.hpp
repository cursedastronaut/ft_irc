/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Protocol.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/07 17:02:26 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/07 23:36:37 by nehuen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "contracts/ClientId.hpp"
#include <string>
#include <vector>

namespace irc
{
	struct	ParsedCommand
	{
		std::string								command;
		std::vector<std::string>	params;
	};
}

struct	ClientIdentity
{
	std::string		nickname;
	std::string		username;
	std::string		host;
};

enum	enqueueResult
{
	Rejected = 0,
	Queued = 1
};

class	protocol
{
	public :
		virtual void	connected(irc::ClientId id) = 0;
		// line has its terminator (CRLF) already stripped by the transport.
		virtual void	online(irc::ClientId id, const std::string &line) = 0;
		virtual void	disconnected(irc::ClientId id, const std::string &reason) = 0;
		virtual				~protocol() {}
};

class transport
{
	public :


		virtual	ClientIdentity 	returnIdentity(irc::ClientId id) const = 0;
		// afterFlush: wait until queued output for this client has been
		// fully written before closing; false closes immediately and
		// discards anything still queued.
		virtual void						requestDisconnection(irc::ClientId id, const std::string &reason, bool afterFlush) = 0;
		// line must not contain '\r'/'\n' (rejected if it does); the
		// transport appends CRLF exactly once when it actually writes it.
		virtual enqueueResult		enqueueLine(irc::ClientId id, const std::string &line) = 0;
		virtual									~transport() {}
};



