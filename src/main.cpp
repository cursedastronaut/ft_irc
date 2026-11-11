#include "net/ServerConfig.hpp"

int	main(int ac, char **av) {
	if (ac != 3){
		std::cerr << "Error: Bad argument" << std::endl;
		std::cerr << "Must use: ./ircserv <port> <password>" << std::endl;
		return (1);
	}

	try {
		int				port = port_parser(av[1]);
		std::string		psw = psw_parser(av[2]);
		ServerConfig	serv(port, psw);

		serv.initListener(INADDR_ANY, LISTEN_BACKLOG);
		
		std::cout << serv << std::endl;	//	Debug

		while (serv.isListening()) {
			int ready = serv.waitForEvents(POLL_TIMEOUT_MS);
			if (ready > 0 && serv.isListenerReady()) {
				int	client_fd = serv.acceptReadyClient();
				(void)client_fd;
			}
			std::cout << serv << std::endl;	//	Debug
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