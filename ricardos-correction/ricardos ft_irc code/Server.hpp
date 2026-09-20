#ifndef SERVER_HPP
# define SERVER_HPP

# include "Channel.hpp"
# include "Client.hpp"
# include "Utils.hpp"
# include <map>
# include <set>
# include <string>
# include <vector>

# define SERVER_NAME	"ircserv"
# define MAX_CLIENTS	200
# define MAX_LINE		4096

typedef std::vector<std::string>	Args;

class Server
{
	public:
		Server(int port, const std::string &password);
		~Server();

		void		run();
		static void	stop(int signal);	// called on ctrl+C

	private:
		int								_port;
		std::string						_password;
		int								_listenFd;
		std::map<int, Client>			_clients;	// socket fd      -> client
		std::map<std::string, Channel>	_channels;	// lowercase name -> channel

		Server(const Server &other);
		Server	&operator=(const Server &other);

		// Server.cpp : the network part
		void	openSocket();
		void	acceptClient();
		void	readFrom(Client &client);
		void	writeTo(Client &client);
		void	removeDeadClients();

		// Helpers.cpp : small tools used by the commands
		void			sendTo(Client &client, const std::string &message);
		void			reply(Client &client, const std::string &code, const std::string &text);
		void			broadcast(Channel &channel, const std::string &message, int exceptFd);
		Client			*findNick(const std::string &nick);
		Channel			*findChannel(const std::string &name);
		std::set<int>	neighbours(Client &client);
		void			leaveChannel(Channel &channel, int fd);
		void			disconnect(Client &client, const std::string &reason);
		void			checkRegistration(Client &client);

		// Commands.cpp : one function per IRC command
		void	handleLine(Client &client, const std::string &line);
		void	cmdPass(Client &client, const Args &args);
		void	cmdNick(Client &client, const Args &args);
		void	cmdUser(Client &client, const Args &args);
		void	cmdPing(Client &client, const Args &args);
		void	cmdQuit(Client &client, const Args &args);
		void	cmdJoin(Client &client, const Args &args);
		void	joinChannel(Client &client, const std::string &name, const std::string &key);
		void	cmdPart(Client &client, const Args &args);
		void	cmdPrivmsg(Client &client, const Args &args);
		void	cmdTopic(Client &client, const Args &args);
		void	cmdKick(Client &client, const Args &args);
		void	cmdInvite(Client &client, const Args &args);
		void	cmdMode(Client &client, const Args &args);
		void	cmdWho(Client &client, const Args &args);
};

#endif
