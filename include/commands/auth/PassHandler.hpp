/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   PassHandler.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/22 00:00:00 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/22 00:00:00 by nehuen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef IRC_COMMANDS_AUTH_PASS_HANDLER_HPP
# define IRC_COMMANDS_AUTH_PASS_HANDLER_HPP

# include "protocol/Dispatcher.hpp"
# include "protocol/ClientRegistry.hpp"
# include "contracts/PasswordSource.hpp"

namespace irc
{
	// PROTO-14 + PROTO-15: Dispatcher guarantees params.size() >= 1 before
	// calling handle() (registered with minParams = 1). params[0] may still
	// be "" (RFC: that's a present-but-empty password, not a missing one).
	class PassHandler : public CommandHandler
	{
		public:
			PassHandler(ClientRegistry &registry, const PasswordSource &source);

			void	handle(ClientId actor, const std::vector<std::string> &params);

		private:
			ClientRegistry			&_registry;
			const PasswordSource	&_source;
	};
}

#endif
