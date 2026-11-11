/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientRegistry.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/10 00:00:00 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/11 19:11:33 by nehuen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "protocol/ClientRegistry.hpp"

namespace irc
{
	bool	ClientRecord::canRegister() const
	{
		if (passwordAccepted && nicknameSet && userSet)
			return true ;
		return false ;
	}
	
	void	ClientRegistry::create(ClientId id)
	{
		ClientRecord cr;

		cr.passwordAccepted = false;
		cr.nicknameSet = false;
		cr.userSet = false;
		cr.id = id;
		_records[id] = cr;
	}

	bool	ClientRegistry::find(ClientId id, ClientRecord &out) const
	{
		std::map<ClientId, ClientRecord>::const_iterator it = _records.find(id);
		if (it == _records.end())
			return false ;
		out = it->second;
		return true ;
	}

	void	ClientRegistry::remove(ClientId id)
	{
		_records.erase(id);
	}
}
