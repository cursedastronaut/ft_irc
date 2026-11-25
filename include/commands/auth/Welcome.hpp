/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Welcome.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/25 14:40:21 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/25 15:43:44 by nehuen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef IRC_COMMANDS_AUTH_WELCOME_HPP
# define IRC_COMMANDS_AUTH_WELCOME_HPP

# include "contracts/ClientId.hpp"
# include "protocol/ClientRegistry.hpp"
# include "contracts/Protocol.hpp"
namespace irc
{
	void	sendWelcome(ClientId id, ClientRegistry &registry, transport &net);
}

#endif
