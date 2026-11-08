#include <iostream>
#include <cassert>
#include <cstring>
#include "net/ServerConfig.hpp"

static void assert_invalid_port(const char *input, const char *label) {
	try {
		port_parser(input);
		std::cerr << "[FAIL] Port wrongly accepted (" << label << ")" << std::endl;
		assert(false);
	} catch (const std::exception &e) {
		std::cout << "[OK] Rejected port (" << label << ") -> " << e.what() << std::endl;
	}
}

int main() {

	std::cout << "---ServerConfig Test--- (Net-06 & Net-07)" << std::endl;
	// 1. NET-06 Tests: Port validation
	assert(port_parser("1") == 1);
	assert(port_parser("65535") == 65535);
	assert(port_parser("6667") == 6667);
	std::cout << "[OK] Valid ports (1, 6667, 65535)" << std::endl;

	assert_invalid_port("0", "lower bound 0");
	assert_invalid_port("65536", "upper bound 65536");
	assert_invalid_port("-1", "negative -1");
	assert_invalid_port("+6667", "plus sign +");
	assert_invalid_port("9999999999999999999999999999", "long overflow");
	assert_invalid_port("2147483648", "INT_MAX + 1");
	assert_invalid_port("6667a", "trailing letters");
	assert_invalid_port("", "empty string");
	assert_invalid_port(NULL, "NULL pointer");

	// 2. NET-07 Tests: Password and ServerConfig construction
	try {
		psw_parser("");
		assert(false);
	} catch (const std::exception &e) {
		std::cout << "[OK] Empty password rejected -> " << e.what() << std::endl;
	}

	const std::string raw_secret = psw_parser("s3cr3t!\t_bytes");
	ServerConfig serv(port_parser("6667"), raw_secret);
	assert(serv.getPort() == 6667);
	assert(serv.getPSW() == raw_secret);
	assert(serv.getClientNb() == 0);

	ServerConfig copy_serv(serv);
	assert(copy_serv.getPort() == 6667);
	assert(copy_serv.getPSW() == raw_secret);
	assert(copy_serv.getClientNb() == 0);
	std::cout << "[OK] ServerConfig stores and copies configuration properly" << std::endl;

	// 3. Increment / Decrement Tests (++ / --) and Bounds (0 .. MAX_CLIENT)
	// Underflow test at 0
	try {
		--serv;
		assert(false);
	} catch (const ServerConfig::ClientUnderFlow &e) {
		std::cout << "[OK] ClientUnderFlow on --serv at 0 -> " << e.what() << std::endl;
	}

	// Pre-increment test (++serv): must equal 1 immediately
	ServerConfig &ref_inc = ++serv;
	assert(&ref_inc == &serv);
	assert(serv.getClientNb() == 1);

	// Post-increment test (serv++): copy returns 1, serv becomes 2
	ServerConfig old_inc = serv++;
	assert(old_inc.getClientNb() == 1);
	assert(serv.getClientNb() == 2);

	// Post-decrement test (serv--): copy returns 2, serv becomes 1
	ServerConfig old_dec = serv--;
	assert(old_dec.getClientNb() == 2);
	assert(serv.getClientNb() == 1);

	// Pre-decrement test (--serv): returns to 0 immediately
	ServerConfig &ref_dec = --serv;
	assert(&ref_dec == &serv);
	assert(serv.getClientNb() == 0);
	std::cout << "[OK] Prefix (++serv, --serv) and postfix (serv++, serv--) behavior verified" << std::endl;

	// Fill up to MAX_CLIENT
	for (size_t i = 0; i < MAX_CLIENT; i++) {
		++serv;
	}
	assert(serv.getClientNb() == MAX_CLIENT);

	// Overflow test beyond MAX_CLIENT
	try {
		serv++;
		assert(false);
	} catch (const ServerConfig::ClientOutOfRange &e) {
		std::cout << "[OK] ClientOutOfRange at MAX_CLIENT (" << MAX_CLIENT << ") -> " << e.what() << std::endl;
	}
	assert(serv.getClientNb() == MAX_CLIENT);

	return (0);
}