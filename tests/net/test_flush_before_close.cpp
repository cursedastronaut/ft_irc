#include "net/ServerConfig.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <ctime>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#define CHECK(cond) do { \
	if (!(cond)) { \
		std::cerr << "FAIL [" << __FILE__ << ":" << __LINE__ << "]: " #cond << std::endl; \
		return (1); \
	} \
} while (0)

namespace {

class RecordingProtocol : public protocol {
public:
	std::vector<std::string>	lines;

	virtual void	connected(irc::ClientId) {}
	virtual void	online(irc::ClientId, const std::string &line) {
		lines.push_back(line);
	}
	virtual void	disconnected(irc::ClientId, const std::string &) {}
};

uint16_t	getBoundPort(int listen_fd) {
	struct sockaddr_in	addr;
	socklen_t			len = sizeof(addr);
	std::memset(&addr, 0, sizeof(addr));
	if (::getsockname(listen_fd, reinterpret_cast<struct sockaddr *>(&addr), &len) < 0)
		return (0);
	return (ntohs(addr.sin_port));
}

int	connectLoopback(uint16_t port) {
	int	fd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		return (-1);
	struct sockaddr_in	addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	if (::connect(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0) {
		::close(fd);
		return (-1);
	}
	return (fd);
}

int	testFinalErrorSentBeforeCloseWhenPeerReads() {
	RecordingProtocol	proto;
	irc::ServerConfig	serv(0, "test_pass", &proto);
	serv.initListener(INADDR_LOOPBACK, 8);
	uint16_t	port = getBoundPort(serv.getListenFd());
	CHECK(port != 0);

	int	peer = connectLoopback(port);
	CHECK(peer >= 0);

	CHECK(serv.waitForEvents(500) > 0);
	CHECK(serv.acceptReadyClient() >= 0);
	irc::ClientId	cid = serv.getLastAcceptedClientId();
	CHECK(cid != 0);

	// Le pair envoie deux commandes avant la demande de déconnexion
	const char	*cmds = "PASS wrong\r\nPRIVMSG #chan :must_not_run\r\n";
	CHECK(::send(peer, cmds, std::strlen(cmds), 0) == static_cast<ssize_t>(std::strlen(cmds)));
	CHECK(serv.waitForEvents(500) > 0);
	CHECK(serv.readAllReadyClients() == 1);
	CHECK(serv.hasClientCompleteLine(cid));

	// Enqueue d'une erreur finale puis demande de flush-before-close
	CHECK(serv.enqueueLine(cid, "ERROR :Closing Link: bad password") == Queued);
	serv.requestDisconnection(cid, "bad password", true);

	// Plus aucune commande acceptée ni nouvelle ligne enqueuée
	CHECK(serv.isClientClosingAfterFlush(cid));
	CHECK(!serv.hasClientCompleteLine(cid));
	CHECK(serv.dispatchAllReadyLines() == 0);
	CHECK(proto.lines.empty());
	CHECK(serv.enqueueLine(cid, "NOTICE * :ignored") == Rejected);
	CHECK(serv.isClientWriteInterestEnabled(cid));

	// Un cycle poll draine la sortie finale puis marque le client pour fermeture
	CHECK(serv.waitForEvents(500) > 0);
	CHECK(serv.writeAllReadyClients() == 1);
	CHECK(serv.isClientMarkedForCleanup(cid));
	CHECK(serv.cleanupMarkedClients() == 1);
	CHECK(serv.getClientNb() == 0);

	// Le pair reçoit l'erreur finale complète suivie d'EOF
	std::string	received;
	char		buf[256];
	for (;;) {
		ssize_t	n = ::recv(peer, buf, sizeof(buf), 0);
		if (n > 0)
			received.append(buf, static_cast<size_t>(n));
		else
			break ;
	}
	::close(peer);
	CHECK(received == "ERROR :Closing Link: bad password\r\n");
	return (0);
}

int	testNonReadingPeerIsBoundedAndClosed() {
	irc::ServerConfig	serv(0, "test_pass");
	serv.setFlushTimeoutSec(1);
	serv.initListener(INADDR_LOOPBACK, 8);
	uint16_t	port = getBoundPort(serv.getListenFd());
	CHECK(port != 0);

	int	peer = connectLoopback(port);
	CHECK(peer >= 0);

	// Réduit le buffer de réception du pair pour saturer rapidement TCP sans lire
	int	small_buf = 4096;
	::setsockopt(peer, SOL_SOCKET, SO_RCVBUF, &small_buf, sizeof(small_buf));

	CHECK(serv.waitForEvents(500) > 0);
	int	server_client_fd = serv.acceptReadyClient();
	CHECK(server_client_fd >= 0);
	::setsockopt(server_client_fd, SOL_SOCKET, SO_SNDBUF, &small_buf, sizeof(small_buf));

	irc::ClientId	cid = serv.getLastAcceptedClientId();
	std::string		chunk(400, 'X');
	for (int i = 0; i < 400; ++i)
		CHECK(serv.enqueueLine(cid, "ERROR :" + chunk) == Queued);

	serv.requestDisconnection(cid, "slow peer", true);
	// Un second appel ne doit pas prolonger/casser l'état de fermeture
	serv.requestDisconnection(cid, "second call", true);
	CHECK(serv.isClientClosingAfterFlush(cid));
	CHECK(serv.getEffectivePollTimeout(5000) <= FLUSH_POLL_TIMEOUT_MS);

	// Le pair ne lit jamais : la borne force la fermeture en <= 3 secondes
	std::time_t	start = std::time(NULL);
	bool		closed = false;
	while (std::time(NULL) - start <= 4) {
		int	timeout = serv.getEffectivePollTimeout(5000);
		int	ready = serv.waitForEvents(timeout);
		if (ready > 0)
			serv.writeAllReadyClients();
		serv.enforceFlushCloseBounds();
		if (serv.cleanupMarkedClients() > 0) {
			closed = true;
			break ;
		}
	}
	::close(peer);
	CHECK(closed);
	CHECK(serv.getClientNb() == 0);
	return (0);
}

int	testDeterministicBoundInjection() {
	irc::ServerConfig	serv(0, "test_pass");
	serv.setFlushTimeoutSec(3);
	serv.initListener(INADDR_LOOPBACK, 8);
	uint16_t	port = getBoundPort(serv.getListenFd());
	CHECK(port != 0);

	int	peer = connectLoopback(port);
	CHECK(peer >= 0);
	CHECK(serv.waitForEvents(500) > 0);
	CHECK(serv.acceptReadyClient() >= 0);
	irc::ClientId	cid = serv.getLastAcceptedClientId();

	CHECK(serv.enqueueLine(cid, "ERROR :pending") == Queued);
	serv.requestDisconnection(cid, "closing", true);

	std::time_t	now = std::time(NULL);
	CHECK(serv.enforceFlushCloseBounds(now + 1) == 0);
	CHECK(!serv.isClientMarkedForCleanup(cid));

	CHECK(serv.enforceFlushCloseBounds(now + 4) == 1);
	CHECK(serv.isClientMarkedForCleanup(cid));
	CHECK(!serv.hasClientPendingOutput(cid));
	CHECK(serv.cleanupMarkedClients() == 1);

	::close(peer);
	return (0);
}

} // namespace

int	main() {
	if (testFinalErrorSentBeforeCloseWhenPeerReads() != 0)
		return (1);
	if (testNonReadingPeerIsBoundedAndClosed() != 0)
		return (1);
	if (testDeterministicBoundInjection() != 0)
		return (1);
	std::cout << "NET-28 flush-before-close tests OK" << std::endl;
	return (0);
}
