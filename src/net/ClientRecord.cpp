#include "net/ClientRecord.hpp"

namespace irc{

netClientRecord::netClientRecord()
	: id(0), fd(-1), in_buf(), out_buf(), out_offset(0),
		marked_for_cleanup(false), close_after_flush(false),
		disconnect_reason() {

}

netClientRecord::netClientRecord(irc::ClientId client_id, int client_fd)
	: id(client_id), fd(client_fd), in_buf(), out_buf(), out_offset(0),
		marked_for_cleanup(false), close_after_flush(false),
		disconnect_reason() {

}

bool	netClientRecord::hasPendingOutput() const {
	return (out_offset < out_buf.size());
}

size_t	netClientRecord::pendingOutputSize() const {
	if (!hasPendingOutput())
		return (0);
	return (out_buf.size() - out_offset);
}

std::string	netClientRecord::pendingOutput() const {
	if (!hasPendingOutput())
		return ("");
	return (out_buf.substr(out_offset));
}

}	//	namespace irc
