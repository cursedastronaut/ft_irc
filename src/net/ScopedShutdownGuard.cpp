#include "net/ScopedShutdownGuard.hpp"

ScopedShutdownGuard::ScopedShutdownGuard(bool &flag): _flag(flag) {
	_flag = true;
}

ScopedShutdownGuard::~ScopedShutdownGuard() {
	_flag = false;
}
