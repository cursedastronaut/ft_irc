/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Reply.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/13 00:00:00 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/16 09:01:23 by nehuen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "protocol/Reply.hpp"
#include <sstream>
#include <iomanip>

static bool	hasForbiddenChar(const std::string &s)
{
	return s.find('\r') != std::string::npos 
	|| s.find('\n') != std::string::npos;
}
namespace irc
{
	ReplyResult	buildNumericReply(int numeric, const std::string &recipient,
					const std::vector<std::string> &params,
					const std::string &text, std::string &out)
	{
		if (hasForbiddenChar(text))
			return ReplyError;
		std::vector<std::string>::const_iterator it;
		for(it = params.begin(); it != params.end(); it++)
		{
			if (hasForbiddenChar(*it))
				return ReplyError;
		}
		std::ostringstream oss;
		oss << ":ircserv ";
		oss << std::setfill('0') << std::setw(3) << numeric;
		
		oss << " " << recipient << " ";
		for(it = params.begin(); it != params.end(); it++)
			oss << *it << " ";
		oss << ":" << text;
		out = oss.str();
		return ReplySuccess;
	}

	ReplyResult	buildEventLine(const ClientIdentity &identity,
					const std::string &command,
					const std::vector<std::string> &params,
					const std::string &text, std::string &out)
	{
		if (hasForbiddenChar(text))
			return ReplyError;
		std::vector<std::string>::const_iterator it;
		for(it = params.begin(); it != params.end(); it++)
		{
			if (hasForbiddenChar(*it))
				return ReplyError;
		}
		std::ostringstream oss;
		oss << ":" << identity.nickname << "!" << 
		identity.username << "@" << identity.host << " " << command << " ";
		for(it = params.begin(); it != params.end(); it++)
			oss << *it << " ";
		oss << ":" << text;
		out = oss.str();
		return ReplySuccess;
	}
}
