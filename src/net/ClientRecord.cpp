#include "net/ClientRecord.hpp"

namespace irc{

netClientRecord::netClientRecord()
	: id(0), fd(-1), in_buf(), out_buf(),
		marked_for_cleanup(false), disconnect_reason(){

}

netClientRecord::netClientRecord(irc::ClientId client_id, int client_fd)
	: id(client_id), fd(client_fd), in_buf(), out_buf(),
		marked_for_cleanup(false), disconnect_reason() {

}

}	//	namespace irc
