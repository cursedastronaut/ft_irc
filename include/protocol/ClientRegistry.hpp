/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientRegistry.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/10 00:00:00 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/11 19:45:32 by nehuen           ###   ########.fr       */
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
		bool		passwordAccepted;
		bool		nicknameSet;
		bool		userSet;

		bool	canRegister() const;
	};

	class ClientRegistry
	{
		public:
			void	create(ClientId id);
			bool	find(ClientId id, ClientRecord &out) const;
			void	remove(ClientId id);
			void	setNickname(ClientId id, const std::string &nickname);
			bool	clientIdForNickname(const std::string &nickname, ClientId &out) const;

		private:
			std::map<ClientId, ClientRecord>	_byID;
			std::map<std::string, ClientId>	_byNickname;
	};
}

#endif
