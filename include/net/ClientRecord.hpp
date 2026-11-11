#ifndef CLIENTRECORD_HPP
# define CLIENTRECORD_HPP

# include <iostream>
# include "contracts/ClientId.hpp"

struct ClientRecord {
	irc::ClientId	id;
	int				fd;
	std::string		in_buf;
	std::string		out_buf;

	ClientRecord();
	ClientRecord(irc::ClientId client_id, int client_fd);
};

#endif //	CLIENTRECORD_HPP