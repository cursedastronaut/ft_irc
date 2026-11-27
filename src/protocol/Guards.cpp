/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Guards.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/19 00:00:00 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/26 16:53:35 by nehuen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "protocol/Guards.hpp"

namespace irc
{
	bool	isPreRegistrationCommand(const std::string &command)
	{
		return (command == "PASS" || command == "NICK" || command == "USER"
			|| command == "QUIT");
	}
}
