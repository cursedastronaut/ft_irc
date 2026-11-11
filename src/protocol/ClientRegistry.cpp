/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientRegistry.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/10 00:00:00 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/10 23:38:27 by nehuen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "protocol/ClientRegistry.hpp"

namespace irc
{
	void	ClientRegistry::create(ClientId id)
	{
		ClientRecord cr;

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
