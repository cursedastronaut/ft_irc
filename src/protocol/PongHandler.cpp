/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   PongHandler.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/17 00:00:00 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/17 14:00:10 by nehuen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "protocol/PongHandler.hpp"

namespace irc
{
	void	PongHandler::handle(ClientId actor, const std::vector<std::string> &params)
	{
		(void)actor;
		(void)params;
	}
}
