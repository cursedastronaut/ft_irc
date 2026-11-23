/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   PassHandler.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nd-angel <nd-angel@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/22 00:00:00 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/23 21:25:25 by nd-angel         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef IRC_COMMANDS_AUTH_PASS_HANDLER_HPP
# define IRC_COMMANDS_AUTH_PASS_HANDLER_HPP

# include "protocol/Dispatcher.hpp"
# include "protocol/ClientRegistry.hpp"
# include "contracts/PasswordSource.hpp"
# include "contracts/Protocol.hpp"

namespace irc
{
	class PassHandler : public CommandHandler
	{
		public:
			PassHandler(ClientRegistry &registry, const PasswordSource &source,
						transport &net);

			void	handle(ClientId actor, const std::vector<std::string> &params);

		private:
			ClientRegistry			&_registry;
			const PasswordSource	&_source;
			transport				&_transport;
	};
}

#endif
