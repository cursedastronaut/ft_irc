/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Reply.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/13 00:00:00 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/13 00:00:00 by nehuen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef IRC_PROTOCOL_REPLY_HPP
# define IRC_PROTOCOL_REPLY_HPP

# include "contracts/Protocol.hpp"
# include <string>
# include <vector>

namespace irc
{
	enum ReplyResult
	{
		ReplySuccess = 0,
		ReplyError = 1
	};

	ReplyResult	buildNumericReply(int numeric, const std::string &recipient,
					const std::vector<std::string> &params,
					const std::string &text, std::string &out);

	ReplyResult	buildEventLine(const ClientIdentity &identity,
					const std::string &command,
					const std::vector<std::string> &params,
					const std::string &text, std::string &out);
}

#endif
