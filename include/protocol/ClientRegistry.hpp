/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientRegistry.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/10 00:00:00 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/25 14:13:57 by nehuen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef IRC_PROTOCOL_CLIENT_REGISTRY_HPP
# define IRC_PROTOCOL_CLIENT_REGISTRY_HPP

# include "contracts/ClientId.hpp"
# include <map>
# include <string>

namespace irc
{
	struct ClientRecord
	{
		ClientId	id;
		std::string	nickname;
		std::string	username;
		std::string	realname;
		bool		passwordAccepted;
		bool		nicknameSet;
		bool		userSet;
		bool		registered;

		bool	canRegister() const;
	};

	class ClientRegistry
	{
		public:
			void	create(ClientId id);
			bool	find(ClientId id, ClientRecord &out) const;
			void	remove(ClientId id);
			bool	setNickname(ClientId id, const std::string &nickname);
			bool	setPasswordAccepted(ClientId id, bool accepted);
			bool	clientIdForNickname(const std::string &nickname, ClientId &out) const;
			void	setRegistered(ClientId id);
			
			// numeric reply target: current nickname, or "*" before NICK
			std::string	recipientFor(ClientId id) const;
			bool	setUser(ClientId id, const std::string &username, const std::string &realname);

		private:
			std::map<ClientId, ClientRecord>	_byID;
			std::map<std::string, ClientId>		_byNickname;
			bool 	maybeRegister(ClientId id);
	};
}

#endif
