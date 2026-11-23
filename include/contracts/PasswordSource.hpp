/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   PasswordSource.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nehuen <nehuen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/22 00:00:00 by nehuen            #+#    #+#             */
/*   Updated: 2026/11/22 00:00:00 by nehuen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef IRC_CONTRACTS_PASSWORD_SOURCE_HPP
# define IRC_CONTRACTS_PASSWORD_SOURCE_HPP

# include <string>

// Read-only view of the server's configured connection password.
// PROTO-15: PassHandler compares against this instead of reaching into
// ServerConfig directly (CORE-01: src/net stays Person 1's territory).
class PasswordSource
{
	public:
		virtual const std::string	&getPassword() const = 0;
		virtual						~PasswordSource() {}
};

#endif
