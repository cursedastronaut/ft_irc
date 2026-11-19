/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Guards.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/19 00:00:00 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/19 00:00:00 by nehuen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef IRC_PROTOCOL_GUARDS_HPP
# define IRC_PROTOCOL_GUARDS_HPP

# include <string>

namespace irc
{
	bool	isPreRegistrationCommand(const std::string &command);
}

#endif
