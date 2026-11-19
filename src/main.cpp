#include "net/ServerConfig.hpp"
#include "protocol/Dispatcher.hpp"
#include "protocol/DispatchProtocol.hpp"

int	main(int ac, char **av) {
	if (ac != 3){
		std::cerr << "Error: Bad argument" << std::endl;
		std::cerr << "Must use: ./ircserv <port> <password>" << std::endl;
		return (1);
	}

	try {
		int				port = irc::port_parser(av[1]);
		std::string		psw = irc::psw_parser(av[2]);
		irc::ServerConfig	serv(port, psw);

		irc::ClientRegistry registry;
		irc::Dispatcher dispatcher;
		irc::DispatchProtocol adaptater(dispatcher, registry);
		serv.setProtocol(&adaptater);

		serv.initListener(INADDR_ANY, LISTEN_BACKLOG);
		std::cout << serv << std::endl;

		while (serv.isListening()) {
			int	timeout = serv.getEffectivePollTimeout(POLL_TIMEOUT_MS);
			int ready = serv.waitForEvents(timeout);
			if (ready > 0) {
				if (serv.isListenerReady()) {
					int	client_fd = serv.acceptReadyClient();
					(void)client_fd;
				}
				serv.readAllReadyClients();
			}
			serv.dispatchAllReadyLines();
			serv.cleanupMarkedClients();
		}
		
	}
	catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
		//	Close ircserv
		return (1);
	}
	//	Close ircserv
	return (0);
}
