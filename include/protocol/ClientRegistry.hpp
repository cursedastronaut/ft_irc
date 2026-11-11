/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientRegistry.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/10 00:00:00 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/10 00:00:00 by nehuen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef IRC_PROTOCOL_CLIENT_REGISTRY_HPP
# define IRC_PROTOCOL_CLIENT_REGISTRY_HPP

# include "contracts/ClientId.hpp"
# include <map>

namespace irc
{
	struct ClientRecord
	{
		ClientId	id;
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

		private:
			std::map<ClientId, ClientRecord>	_records;
	};
}

#endif
