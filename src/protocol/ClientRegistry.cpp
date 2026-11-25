/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientRegistry.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/10 00:00:00 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/25 15:57:35 by nehuen           ###   ########.fr       */
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

	bool ClientRegistry::setNickname(ClientId id, const std::string &nickname)
	{
		std::map<ClientId, ClientRecord>::iterator it = _byID.find(id);

		if (it == _byID.end())
			return false;
		// PROTO-19: free the old key BEFORE writing the new one, so that
		// bob -> BOB (same canonical key) does not erase the fresh entry
		if (it->second.nicknameSet)
			_byNickname.erase(irc::canonicalizeIrcName(it->second.nickname));
		it->second.nickname = nickname;
		it->second.nicknameSet = true;
		_byNickname[irc::canonicalizeIrcName(nickname)] = id;
		return maybeRegister(id);
	}

	bool ClientRegistry::setPasswordAccepted(ClientId id, bool accepted)
	{
		// TODO PROTO-15: même pattern que setNickname (find, bail si absent,
		if (_byID.find(id) == _byID.end())
			return false;
		_byID[id].passwordAccepted = accepted;
		return maybeRegister(id);
	}	
	
	bool	ClientRegistry::setUser(ClientId id, const std::string &username, const std::string &realname)
	{
		if (_byID.find(id) == _byID.end())
			return false;
		_byID[id].username = username;
		_byID[id].realname = realname;
		_byID[id].userSet = true;
		return maybeRegister(id);
	}


	bool ClientRegistry::clientIdForNickname(const std::string &nickname, ClientId &out) const	
	
	{
		std::map<std::string, ClientId>::const_iterator it = _byNickname.find(irc::canonicalizeIrcName(nickname));
		
		if (it == _byNickname.end())
			return false ;
		out = it->second;
		return true ;
	}

    bool ClientRegistry::isRegistered(ClientId id) const
    {
        std::map<ClientId, ClientRecord>::const_iterator it = _byID.find(id);
        return it != _byID.end() && it->second.registered;
    }

    bool ClientRegistry::identityOf(ClientId id, ClientIdentity &out) const
    {
        std::map<ClientId, ClientRecord>::const_iterator it = _byID.find(id);
        if (it == _byID.end() || !it->second.registered)
            return false;
        out.nickname = it->second.nickname;
        out.username = it->second.username;
        out.host = "ircserv"; // Server-owned host mask, independent of USER fields.
        return true;
    }

	void	ClientRegistry::setRegistered(ClientId id)
	{
		std::map<ClientId, ClientRecord>::iterator it = _byID.find(id);

		if (it == _byID.end())
			return ;
		it->second.registered = true;
	}

	bool 	ClientRegistry::maybeRegister(ClientId id)
	{
		if (!_byID[id].registered && _byID[id].canRegister())
		{
			_byID[id].registered = true;
			return true;
		}
		return false;
	}
	std::string	ClientRegistry::recipientFor(ClientId id) const
	{
		std::map<ClientId, ClientRecord>::const_iterator it = _byID.find(id);

		if (it == _byID.end() || !it->second.nicknameSet)
			return "*";
		return it->second.nickname;
	}
}
