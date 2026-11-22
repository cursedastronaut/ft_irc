/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   PassHandler.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/22 00:00:00 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/22 21:54:54 by nehuen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "commands/auth/PassHandler.hpp"

namespace irc
{
	PassHandler::PassHandler(ClientRegistry &registry, const PasswordSource &source)
		: _registry(registry), _source(source) {
	}

	void	PassHandler::handle(ClientId actor, const std::vector<std::string> &params)
	{	
	
		if (params[0] == _source.getPassword())
			_registry.setPasswordAccepted(actor, true);
		else 
			_registry.setPasswordAccepted(actor, false);
	}
}
