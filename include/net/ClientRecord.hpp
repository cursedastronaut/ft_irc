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
	bool			marked_for_cleanup;
	bool			disconnecting;
	std::string		disconnect_reason;

	netClientRecord();
	netClientRecord(irc::ClientId client_id, int client_fd);
};

}	//	namespace irc

#endif //	CLIENTRECORD_HPP
