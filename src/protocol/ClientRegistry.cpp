/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientRegistry.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nd-angel <nd-angel@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/10 00:00:00 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/23 16:31:31 by nd-angel         ###   ########.fr       */
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
		cr.registered = false;
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
		std::map<ClientId, ClientRecord>::iterator it = _byID.find(id);

		if (it == _byID.end())
			return ;
		if (it->second.nicknameSet)
			_byNickname.erase(irc::canonicalizeIrcName(it->second.nickname));
		_byID.erase(it);
	}

	void ClientRegistry::setNickname(ClientId id, const std::string &nickname)
	{
		std::map<ClientId, ClientRecord>::iterator it = _byID.find(id);

		if (it == _byID.end())
			return ;
		// PROTO-19: free the old key BEFORE writing the new one, so that
		// bob -> BOB (same canonical key) does not erase the fresh entry
		if (it->second.nicknameSet)
			_byNickname.erase(irc::canonicalizeIrcName(it->second.nickname));
		it->second.nickname = nickname;
		it->second.nicknameSet = true;
		_byNickname[irc::canonicalizeIrcName(nickname)] = id;
	}

	void ClientRegistry::setPasswordAccepted(ClientId id, bool accepted)
	{
		// TODO PROTO-15: même pattern que setNickname (find, bail si absent,
		if (_byID.find(id) == _byID.end())
			return ;
		_byID[id].passwordAccepted = accepted;
	}	
	

	bool ClientRegistry::clientIdForNickname(const std::string &nickname, ClientId &out) const	
	
	{
		std::map<std::string, ClientId>::const_iterator it = _byNickname.find(irc::canonicalizeIrcName(nickname));
		
		if (it == _byNickname.end())
			return false ;
		out = it->second;
		return true ;
	}

	void	ClientRegistry::setRegistered(ClientId id)
	{
		std::map<ClientId, ClientRecord>::iterator it = _byID.find(id);

		if (it == _byID.end())
			return ;
		it->second.registered = true;
	}

	std::string	ClientRegistry::recipientFor(ClientId id) const
	{
		std::map<ClientId, ClientRecord>::const_iterator it = _byID.find(id);

		if (it == _byID.end() || !it->second.nicknameSet)
			return "*";
		return it->second.nickname;
	}
}
