#include "net/ServerConfig.hpp"
#include "protocol/Dispatcher.hpp"
#include "protocol/DispatchProtocol.hpp"
#include "commands/auth/PassHandler.hpp"
#include "commands/auth/NickHandler.hpp"
#include "commands/auth/UserHandler.hpp"
#include "commands/channel/Join.hpp"
#include "commands/channel/Names.hpp"
#include "commands/channel/Who.hpp"
#include "commands/channel/Mode.hpp"
#include "commands/channel/Part.hpp"
#include "commands/channel/Topic.hpp"
#include "commands/channel/Kick.hpp"
#include "commands/channel/Invite.hpp"

namespace
{
class ServerProtocol : public irc::DispatchProtocol
{
public:
    ServerProtocol(irc::Dispatcher &dispatcher, irc::ClientRegistry &users,
                   irc::ServerConfig &server, irc::ChannelRegistry &channels)
        : irc::DispatchProtocol(dispatcher, users, server), server_(server), channels_(channels)
    {
        server_.setProtocol(this);
    }
    ~ServerProtocol() { server_.setProtocol(0); }
    void disconnected(irc::ClientId id, const std::string &reason)
    {
        channels_.removeClient(id);
        irc::DispatchProtocol::disconnected(id, reason);
    }
private:
    irc::ServerConfig &server_;
    irc::ChannelRegistry &channels_;
};
}

int	main(int ac, char **av) {
	if (ac != 3){
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
		irc::ServerConfig		serv(port, psw);
		irc::ClientRegistry		registry;
		irc::Dispatcher			dispatcher;
		irc::ChannelRegistry channels;
        ServerProtocol protocol(dispatcher, registry, serv, channels);
        irc::ChannelContext channelContext(channels, registry, serv);


		irc::PassHandler passHandler(registry, serv, serv);
		dispatcher.registerHandler("PASS", &passHandler, 1);
		irc::NickHandler nickHandler(registry, serv);
		dispatcher.registerHandler("NICK", &nickHandler, 0);
		irc::UserHandler userHandler(registry, serv);
		dispatcher.registerHandler("USER", &userHandler, 4);

        irc::Who who(channelContext);
        dispatcher.registerHandler("WHO", &who);
        irc::Names names(channelContext);
        dispatcher.registerHandler("NAMES", &names);
        irc::Join join(channelContext);
        dispatcher.registerHandler("JOIN", &join);
        irc::Mode mode(channelContext);
        dispatcher.registerHandler("MODE", &mode);
        irc::Part part(channelContext);
        dispatcher.registerHandler("PART", &part);
        irc::Topic topic(channelContext);
        dispatcher.registerHandler("TOPIC", &topic);
        irc::Kick kick(channelContext);
        dispatcher.registerHandler("KICK", &kick);
        irc::Invite invite(channelContext);
        dispatcher.registerHandler("INVITE", &invite);

		serv.initListener(INADDR_ANY, LISTEN_BACKLOG);
		std::cout << serv << std::endl;

		while (serv.isListening() && !irc::ServerConfig::isStopRequested()) {
			int	timeout = serv.getEffectivePollTimeout(POLL_TIMEOUT_MS);
			int ready = serv.waitForEvents(timeout);
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
				break;
			serv.dispatchAllReadyLines();
			serv.markErroredClientsForCleanup();
			serv.enforceFlushCloseBounds();
			serv.cleanupMarkedClients();
		}
		serv.closeListener();
	}
	catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
		//	Close ircserv
		return (1);
	}
	//	Close ircserv
	return (0);
}
