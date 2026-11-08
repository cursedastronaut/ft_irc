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

		//	Debug / Test
		std::cout << serv << std::endl;
		//	~Debug / ~Test
		
		//	Authentification
		//	ircserv core
	}
	catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
		//	Close ircserv
		return (1);
	}
	//	Close ircserv
	return (0);
}