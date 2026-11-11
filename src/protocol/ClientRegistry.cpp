/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientRegistry.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/10 00:00:00 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/11 19:46:51 by nehuen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "protocol/ClientRegistry.hpp"
#include "contracts/IrcCaseMapping.hpp"
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
		_byID[id] = cr;
	}

	bool	ClientRegistry::find(ClientId id, ClientRecord &out) const
	{
		std::map<ClientId, ClientRecord>::const_iterator it = _byID.find(id);
		if (it == _byID.end())
			return false ;
		out = it->second;
		return true ;
	}

	void	ClientRegistry::remove(ClientId id)
	{
		_byID.erase(id);
	}

	void ClientRegistry::setNickname(ClientId id, const std::string &nickname)
	{
		std::map<ClientId, ClientRecord>::const_iterator it = _byID.find(id);
		
		if (it == _byID.end())
			return ;
		_byID[id].nickname = nickname;
		_byNickname[irc::canonicalizeIrcName(nickname)] = id;
	}

	bool ClientRegistry::clientIdForNickname(const std::string &nickname, ClientId &out) const
	{
		std::map<std::string, ClientId>::const_iterator it = _byNickname.find(irc::canonicalizeIrcName(nickname));
		
		if (it == _byNickname.end())
			return false ;
		out = it->second;
		return true ;
	}
}
