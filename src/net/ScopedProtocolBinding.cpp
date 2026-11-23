#include "net/ScopedProtocolBinding.hpp"

namespace irc
{

ScopedProtocolBinding::ScopedProtocolBinding(irc::ServerConfig &serv, protocol *proto)
	: _serv(serv) {
	_serv.setProtocol(proto);
}

ScopedProtocolBinding::~ScopedProtocolBinding() {
	try {
		_serv.closeListener();
	}
	catch (...) {
	}
	_serv.setProtocol(NULL);
}

}	//	namespace irc
