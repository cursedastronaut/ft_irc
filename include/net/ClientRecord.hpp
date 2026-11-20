#ifndef CLIENTRECORD_HPP
# define CLIENTRECORD_HPP

# include <iostream>
# include "contracts/ClientId.hpp"

namespace irc 
{

struct netClientRecord {
	irc::ClientId	id;
	int				fd;
	std::string		in_buf;
	std::string		out_buf;
	size_t			out_offset;
	bool			marked_for_cleanup;
	bool			disconnecting;
	bool			close_after_flush;
	std::string		disconnect_reason;

	netClientRecord();
	netClientRecord(irc::ClientId client_id, int client_fd);

	bool		hasPendingOutput() const;
	size_t		pendingOutputSize() const;
	std::string	pendingOutput() const;
};

}	//	namespace irc

#endif //	CLIENTRECORD_HPP
