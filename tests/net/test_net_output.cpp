#include "net/ServerConfig.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

#define CHECK(cond) do { \
	if (!(cond)) { \
		std::cerr << "FAIL: " #cond " at " << __FILE__ << ":" << __LINE__ << std::endl; \
		std::exit(1); \
	} \
} while (0)

static int connectPeer(int port) {
	int fd = ::socket(AF_INET, SOCK_STREAM, 0);
	CHECK(fd >= 0);
	struct sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(static_cast<uint16_t>(port));
	CHECK(::connect(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) == 0);
	return (fd);
}

static void testNet22And23() {
	irc::ServerConfig serv(0, "pw");
	serv.initListener(INADDR_LOOPBACK, 8);
	struct sockaddr_in bound;
	socklen_t len = sizeof(bound);
	CHECK(::getsockname(serv.getListenFd(), reinterpret_cast<struct sockaddr *>(&bound), &len) == 0);
	int port = ntohs(bound.sin_port);

	int peer = connectPeer(port);
	CHECK(serv.waitForEvents(50) > 0);
	CHECK(serv.acceptReadyClient() >= 0);
	irc::ClientId id = serv.getLastAcceptedClientId();

	// NET-23: idle client has no POLLOUT interest
	CHECK(!serv.isClientWriteInterestEnabled(id));
	std::cout << "[OK] idle client has no POLLOUT interest" << std::endl;

	// NET-22: reject embedded CR or LF without partial insertion
	transport *tr = &serv;
	CHECK(tr->enqueueLine(id, "bad\rline") == Rejected);
	CHECK(tr->enqueueLine(id, "bad\nline") == Rejected);
	CHECK(!serv.hasClientPendingOutput(id));
	CHECK(!serv.isClientWriteInterestEnabled(id));
	std::cout << "[OK] reject embedded CR or LF without partial insertion" << std::endl;

	// NET-22 + NET-23: valid line appends CRLF once and enables POLLOUT
	CHECK(tr->enqueueLine(id, ":ircserv 001 nick :Welcome") == Queued);
	CHECK(serv.getClientById(id)->pendingOutput() == ":ircserv 001 nick :Welcome\r\n");
	CHECK(serv.isClientWriteInterestEnabled(id));

	::close(peer);
	std::cout << "[OK] valid line appends CRLF once and enables POLLOUT" << std::endl;
}

static void testNet24And25() {
	irc::ServerConfig serv(0, "pw");
	serv.initListener(INADDR_LOOPBACK, 8);
	struct sockaddr_in bound;
	socklen_t len = sizeof(bound);
	CHECK(::getsockname(serv.getListenFd(), reinterpret_cast<struct sockaddr *>(&bound), &len) == 0);
	int port = ntohs(bound.sin_port);

	// 1. Test NET-24 (partial send offset) + NET-25 (injected send <= 0)
	int peer1 = connectPeer(port);
	CHECK(serv.waitForEvents(50) > 0);
	CHECK(serv.acceptReadyClient() >= 0);
	irc::ClientId id1 = serv.getLastAcceptedClientId();

	CHECK(serv.enqueueLine(id1, "ABCDEF") == Queued); // 8 bytes with \r\n

	// Partial send advances offset and retains remainder
	CHECK(serv.handleSendResult(id1, 3) == 3);
	CHECK(serv.getClientOutputOffset(id1) == 3);
	CHECK(serv.getClientById(id1)->pendingOutput() == "DEF\r\n");
	CHECK(serv.isClientWriteInterestEnabled(id1));

	// Next write must wait for poll readiness again
	CHECK(serv.writeReadyClient(id1) == -1);

	// Complete remaining bytes -> clears queue and POLLOUT
	CHECK(serv.handleSendResult(id1, 5) == 5);
	CHECK(!serv.hasClientPendingOutput(id1));
	CHECK(!serv.isClientWriteInterestEnabled(id1));

	// Injected send <= 0 marks client for cleanup without errno branching
	CHECK(serv.injectSendResult(id1, 0) == 0);
	CHECK(serv.isClientMarkedForCleanup(id1));
	CHECK(serv.cleanupMarkedClients() == 1);
	::close(peer1);
	std::cout << "[OK] Test NET-24 (partial send offset) + NET-25 (injected send <= 0)" << std::endl;

	// 2. Test NET-25: dead peer write triggers RST/SIGPIPE without killing process
	int peer2 = connectPeer(port);
	CHECK(serv.waitForEvents(50) > 0);
	CHECK(serv.acceptReadyClient() >= 0);
	irc::ClientId id2 = serv.getLastAcceptedClientId();

	::close(peer2);
	// First write sends bytes after peer FIN, provoking TCP RST from peer kernel
	CHECK(serv.enqueueLine(id2, "PING :dead1") == Queued);
	CHECK(serv.waitForEvents(50) > 0);
	serv.writeAllReadyClients();

	// Second write hits RST/EPIPE (SIGPIPE must be ignored) and marks cleanup
	if (!serv.isClientMarkedForCleanup(id2)) {
		CHECK(serv.enqueueLine(id2, "PING :dead2") == Queued);
		CHECK(serv.waitForEvents(50) > 0);
		serv.writeAllReadyClients();
		serv.markErroredClientsForCleanup();
	}
	CHECK(serv.isClientMarkedForCleanup(id2));
	CHECK(serv.cleanupMarkedClients() == 1);
	std::cout << "[OK] Test NET-25: dead peer write triggers RST/SIGPIPE without killing process" << std::endl;
}

static void testNet26() {
	irc::ServerConfig serv(0, "pw");
	serv.initListener(INADDR_LOOPBACK, 8);
	struct sockaddr_in bound;
	socklen_t len = sizeof(bound);
	CHECK(::getsockname(serv.getListenFd(), reinterpret_cast<struct sockaddr *>(&bound), &len) == 0);
	int port = ntohs(bound.sin_port);

	int p1 = connectPeer(port);
	CHECK(serv.waitForEvents(50) > 0);
	CHECK(serv.acceptReadyClient() >= 0);
	irc::ClientId id1 = serv.getLastAcceptedClientId();

	int p2 = connectPeer(port);
	CHECK(serv.waitForEvents(50) > 0);
	CHECK(serv.acceptReadyClient() >= 0);
	irc::ClientId id2 = serv.getLastAcceptedClientId();

	int p3 = connectPeer(port);
	CHECK(serv.waitForEvents(50) > 0);
	CHECK(serv.acceptReadyClient() >= 0);
	irc::ClientId id3 = serv.getLastAcceptedClientId();

	// First client closes while second and third send data
	::close(p1);
	CHECK(::send(p2, "PING :two\r\n", 11, 0) == 11);
	CHECK(::send(p3, "PING :three\r\n", 13, 0) == 13);
	
	CHECK(serv.waitForEvents(50) > 0);
	serv.readAllReadyClients();
	// Duplicate removal requests for id1 must stay idempotent
	serv.markClientForCleanup(id1, "Duplicate request");
	
	// Second and third clients were not skipped
	CHECK(serv.hasClientCompleteLine(id2));
	CHECK(serv.hasClientCompleteLine(id3));
	CHECK(serv.cleanupMarkedClients() == 1);
	CHECK(serv.getClientNb() == 2);
	
	::close(p2);
	::close(p3);
	std::cout << "[OK] First client closes while second and third send data" << std::endl;
}

int main() {
	testNet22And23();
	std::cout << "NET-22 && 23 OK" << std::endl;
	testNet24And25();
	std::cout << "NET-24 && 25 OK" << std::endl;
	testNet26();
	std::cout << "NET-26 OK" << std::endl;
	return (0);
}