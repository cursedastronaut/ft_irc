/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Dispatcher.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/16 00:00:00 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/17 10:18:48 by nehuen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef IRC_PROTOCOL_DISPATCHER_HPP
# define IRC_PROTOCOL_DISPATCHER_HPP

# include "contracts/ClientId.hpp"
# include "contracts/Protocol.hpp"
# include <map>
# include <string>
# include <vector>

namespace irc
{
	class CommandHandler
	{
		public:
			virtual void	handle(ClientId actor,
							const std::vector<std::string> &params) = 0;
			virtual			~CommandHandler() {}
	};

	enum DispatchResult
	{
		DispatchHandled = 0,
		DispatchUnknownCommand = 1
	};

	class Dispatcher
	{
		public:
			void			registerHandler(const std::string &command,
							CommandHandler *handler);
			DispatchResult	dispatch(ClientId actor,
							const ParsedCommand &command) const;
		private:
			std::map<std::string, CommandHandler *>	_handlers;
	};
}

#endif
