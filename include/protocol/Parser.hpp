/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Parser.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/09 11:30:11 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/09 11:33:14 by nehuen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef IRC_PROTOCOL_PARSER_HPP
# define IRC_PROTOCOL_PARSER_HPP

# include "contracts/Protocol.hpp"
# include <string>

namespace irc
{
	enum ParseResult 
	{
		ParseSuccess = 0, 
		ParseIgnore = 1, 
		ParseError = 2 
	};

	ParseResult	parseCommand(const std::string &line, ParsedCommand &out);
}

#endif
