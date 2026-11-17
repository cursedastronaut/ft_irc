/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   PongHandler.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/17 00:00:00 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/17 00:00:00 by nehuen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef IRC_PROTOCOL_PONG_HANDLER_HPP
# define IRC_PROTOCOL_PONG_HANDLER_HPP

# include "protocol/Dispatcher.hpp"

namespace irc
{
	class PongHandler : public CommandHandler
	{
		public:
			void	handle(ClientId actor, const std::vector<std::string> &params);
	};
}

#endif
