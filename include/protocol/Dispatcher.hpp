/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Dispatcher.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/16 00:00:00 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/26 10:38:25 by nehuen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef IRC_PROTOCOL_DISPATCHER_HPP
# define IRC_PROTOCOL_DISPATCHER_HPP

# include "contracts/ClientId.hpp"
# include "contracts/Protocol.hpp"
# include <cstddef>
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
		DispatchUnknownCommand = 1,
		DispatchNeedMoreParams = 2
	};

	class Dispatcher
	{
		public:
			void			registerHandler(const std::string &command,
							CommandHandler *handler,
							std::size_t minParams = 0);
			DispatchResult	dispatch(ClientId actor,
							const ParsedCommand &command) const;
		private:
			std::map<std::string, CommandHandler *>	_handlers;
			std::map<std::string, std::size_t>		_minParams;
	};
}

#endif
