#include "net/ClientRecord.hpp"

ClientRecord::ClientRecord(): id(0), fd(-1), in_buf(), out_buf() {

}

ClientRecord::ClientRecord(irc::ClientId client_id, int client_fd)
	: id(client_id), fd(client_fd), in_buf(), out_buf() {

}