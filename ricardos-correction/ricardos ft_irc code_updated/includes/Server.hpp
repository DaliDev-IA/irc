#ifndef SERVER_HPP
# define SERVER_HPP

# include <csignal>
# include <map>
# include <set>
# include <string>
# include <vector>
# include "Client.hpp"
# include "Channel.hpp"

# define SERVER_NAME "ircserv"

typedef std::vector<std::string>	Args;

extern volatile sig_atomic_t		g_running;

class Server
{
	public:
		Server(int port, const std::string &password);
		~Server();

		void	run();

	private:
		int								_port;
		std::string						_password;
		int								_listenFd;
		std::map<int, Client>			_clients;	// fd -> client
		std::map<std::string, Channel>	_channels;	// lowercase name -> channel

		Server(const Server &);
		Server	&operator=(const Server &);

		// Server.cpp : socket + poll loop
		void	setup();
		void	acceptClient();
		void	readClient(Client &client);
		void	writeClient(Client &client);
		void	removeDeadClients();

		// Utils.cpp : small tools used by the commands
		void			sendTo(Client &client, const std::string &msg);
		void			reply(Client &client, const std::string &code, const std::string &text);
		void			broadcast(Channel &chan, const std::string &msg, int exceptFd);
		Client			*findNick(const std::string &nick);
		Channel			*findChannel(const std::string &name);
		std::set<int>	neighbours(Client &client);
		void			leaveChannel(Channel &chan, int fd);
		void			disconnect(Client &client, const std::string &reason);
		void			tryRegister(Client &client);

		// Commands.cpp : connection + basic commands
		void	handleLine(Client &client, const std::string &line);
		void	cmdPass(Client &client, const Args &args);
		void	cmdNick(Client &client, const Args &args);
		void	cmdUser(Client &client, const Args &args);
		void	cmdPing(Client &client, const Args &args);
		void	cmdQuit(Client &client, const Args &args);
		void	cmdJoin(Client &client, const Args &args);
		void	joinOne(Client &client, const std::string &name, const std::string &key);
		void	cmdPart(Client &client, const Args &args);
		void	cmdPrivmsg(Client &client, const Args &args);

		// Operators.cpp : channel operator commands
		void	cmdTopic(Client &client, const Args &args);
		void	cmdKick(Client &client, const Args &args);
		void	cmdInvite(Client &client, const Args &args);
		void	cmdMode(Client &client, const Args &args);
};

// Utils.cpp : string helpers
std::string	lower(std::string s);
std::string	upper(std::string s);
bool		isNumber(const std::string &s);
Args		split(const std::string &s, char sep);
Args		parseLine(const std::string &line);

#endif
