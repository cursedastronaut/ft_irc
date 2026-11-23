#include "net/ServerConfig.hpp"
#include "protocol/Dispatcher.hpp"
#include "protocol/DispatchProtocol.hpp"
#include "commands/auth/PassHandler.hpp"
#include "net/ScopedProtocolBinding.hpp"

int	main(int ac, char **av) {
	if (ac != 3) {
		std::cerr << "Error: Bad argument" << std::endl;
		std::cerr << "Must use: ./ircserv <port> <password>" << std::endl;
		return (1);
	}

	try {
		if (!irc::ServerConfig::installShutdownSignals()) {
			std::cerr << "Error: Failed to install shutdown signal handlers" << std::endl;
			return (1);
		}

		int						port = irc::port_parser(av[1]);
		std::string				psw = irc::psw_parser(av[2]);
		irc::ClientRegistry		registry;
		irc::Dispatcher			dispatcher;
		irc::ServerConfig		serv(port, psw);
		irc::DispatchProtocol	adaptater(dispatcher, registry, serv);
		irc::PassHandler		passHandler(registry, serv);

		dispatcher.registerHandler("PASS", &passHandler, 1);
		irc::ScopedProtocolBinding	protocol_binding(serv, &adaptater);

		serv.initListener(INADDR_ANY, LISTEN_BACKLOG);
		std::cout << serv << std::endl;

		while (serv.isListening() && !irc::ServerConfig::isStopRequested()) {
			int	timeout = serv.getEffectivePollTimeout(POLL_TIMEOUT_MS);
			int	ready = serv.waitForEvents(timeout);
			if (irc::ServerConfig::isStopRequested())
				break ;
			if (ready > 0) {
				if (serv.isListenerReady()) {
					int	client_fd = serv.acceptReadyClient();
					(void)client_fd;
				}
				serv.readAllReadyClients();
				serv.writeAllReadyClients();
			}
			if (irc::ServerConfig::isStopRequested())
				break ;
			serv.dispatchAllReadyLines();
			serv.markErroredClientsForCleanup();
			serv.enforceFlushCloseBounds();
			serv.cleanupMarkedClients();
		}
		serv.closeListener();
	}
	catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
		return (1);
	}
	return (0);
}
