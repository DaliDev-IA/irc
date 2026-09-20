#include "Server.hpp"
#include <cctype>
#include <cstdlib>

// Called for every complete line we receive from a client.
void	Server::handleLine(Client &client, const std::string &line)
{
	Args args = splitLine(line);
	if (args.empty())
		return;
	std::string command = upper(args[0]);

	if (command == "CAP" || command == "PONG")
		return;		// sent automatically by IRC clients, nothing to do
	else if (command == "PASS")
		cmdPass(client, args);
	else if (command == "NICK")
		cmdNick(client, args);
	else if (command == "USER")
		cmdUser(client, args);
	else if (command == "QUIT")
		cmdQuit(client, args);
	else if (!client.registered)
		reply(client, "451", ":You have not registered");
	else if (command == "PING")
		cmdPing(client, args);
	else if (command == "JOIN")
		cmdJoin(client, args);
	else if (command == "PART")
		cmdPart(client, args);
	else if (command == "PRIVMSG" || command == "NOTICE")
		cmdPrivmsg(client, args);
	else if (command == "TOPIC")
		cmdTopic(client, args);
	else if (command == "KICK")
		cmdKick(client, args);
	else if (command == "INVITE")
		cmdInvite(client, args);
	else if (command == "MODE")
		cmdMode(client, args);
	else if (command == "WHO")
		cmdWho(client, args);
	else
		reply(client, "421", command + " :Unknown command");
}

/* ************************** connecting ************************** */

// PASS <password>
void	Server::cmdPass(Client &client, const Args &args)
{
	if (client.registered)
		reply(client, "462", ":You may not reregister");
	else if (args.size() < 2)
		reply(client, "461", "PASS :Not enough parameters");
	else if (args[1] != _password)
	{
		client.passOk = false;
		reply(client, "464", ":Password incorrect");
	}
	else
	{
		client.passOk = true;
		checkRegistration(client);
	}
}

static bool	isValidNick(const std::string &nick)
{
	const std::string special = "[]\\`_^{|}-";

	if (nick.empty() || nick.size() > 30 || std::isdigit(nick[0]) || nick[0] == '-')
		return false;
	for (size_t i = 0; i < nick.size(); i++)
		if (!std::isalnum(static_cast<unsigned char>(nick[i])) && special.find(nick[i]) == std::string::npos)
			return false;
	return true;
}

// NICK <nickname>
void	Server::cmdNick(Client &client, const Args &args)
{
	if (args.size() < 2)
		return reply(client, "431", ":No nickname given");
	if (!isValidNick(args[1]))
		return reply(client, "432", args[1] + " :Erroneous nickname");

	Client *owner = findNick(args[1]);
	if (owner != NULL && owner != &client)
		return reply(client, "433", args[1] + " :Nickname is already in use");

	if (client.registered)
	{
		// tell the client and everybody who can see it about the new name
		std::string		message = ":" + client.prefix() + " NICK :" + args[1];
		std::set<int>	others = neighbours(client);

		sendTo(client, message);
		for (std::set<int>::iterator it = others.begin(); it != others.end(); ++it)
			sendTo(_clients.find(*it)->second, message);
	}
	client.nick = args[1];
	checkRegistration(client);
}

// USER <username> 0 * :<real name>
void	Server::cmdUser(Client &client, const Args &args)
{
	if (client.registered)
		return reply(client, "462", ":You may not reregister");
	if (args.size() < 5)
		return reply(client, "461", "USER :Not enough parameters");
	client.user = args[1];
	client.realname = args[4];
	checkRegistration(client);
}

// PING <token>   (clients use it to check that we are still alive)
void	Server::cmdPing(Client &client, const Args &args)
{
	if (args.size() < 2)
		return reply(client, "409", ":No origin specified");
	sendTo(client, std::string(":") + SERVER_NAME + " PONG " + SERVER_NAME + " :" + args[1]);
}

// QUIT [:reason]
void	Server::cmdQuit(Client &client, const Args &args)
{
	disconnect(client, args.size() > 1 ? "Quit: " + args[1] : "Quit");
}

/* *************************** channels *************************** */

// JOIN <#chan>[,<#chan>...] [<key>[,<key>...]]
void	Server::cmdJoin(Client &client, const Args &args)
{
	if (args.size() < 2)
		return reply(client, "461", "JOIN :Not enough parameters");

	Args names = split(args[1], ',');
	Args keys;
	if (args.size() > 2)
		keys = split(args[2], ',');

	for (size_t i = 0; i < names.size(); i++)
		joinChannel(client, names[i], i < keys.size() ? keys[i] : "");
}

void	Server::joinChannel(Client &client, const std::string &name, const std::string &key)
{
	if (name.size() < 2 || name.size() > 50 || name[0] != '#' || name.find('\a') != std::string::npos)
		return reply(client, "476", name + " :Bad channel name (it must start with #)");

	Channel &channel = _channels[lower(name)];	// created here if it is new
	bool isNew = channel.members.empty();

	if (isNew)
		channel.name = name;
	if (channel.has(client.fd))
		return;
	if (channel.inviteOnly && channel.invited.count(client.fd) == 0)
		return reply(client, "473", channel.name + " :Cannot join channel (+i)");
	if (!channel.key.empty() && key != channel.key)
		return reply(client, "475", channel.name + " :Cannot join channel (+k)");
	if (channel.limit > 0 && channel.members.size() >= channel.limit)
		return reply(client, "471", channel.name + " :Cannot join channel (+l)");

	channel.members.insert(client.fd);
	channel.invited.erase(client.fd);
	if (isNew)
		channel.operators.insert(client.fd);	// the creator is the first operator

	broadcast(channel, ":" + client.prefix() + " JOIN " + channel.name, -1);
	if (!channel.topic.empty())
		reply(client, "332", channel.name + " :" + channel.topic);

	// the list of people in the channel, operators are shown with a '@'
	std::string list;
	for (std::set<int>::iterator it = channel.members.begin(); it != channel.members.end(); ++it)
		list += (channel.isOperator(*it) ? "@" : "") + _clients.find(*it)->second.nick + " ";
	reply(client, "353", "= " + channel.name + " :" + list);
	reply(client, "366", channel.name + " :End of /NAMES list");
}

// PART <#chan> [:reason]
void	Server::cmdPart(Client &client, const Args &args)
{
	if (args.size() < 2)
		return reply(client, "461", "PART :Not enough parameters");

	Channel *channel = findChannel(args[1]);
	if (channel == NULL)
		return reply(client, "403", args[1] + " :No such channel");
	if (!channel->has(client.fd))
		return reply(client, "442", channel->name + " :You're not on that channel");

	std::string reason = args.size() > 2 ? " :" + args[2] : "";
	broadcast(*channel, ":" + client.prefix() + " PART " + channel->name + reason, -1);
	leaveChannel(*channel, client.fd);
}

// PRIVMSG <nick or #chan> :<text>
// NOTICE is the same thing, but it never answers with an error.
void	Server::cmdPrivmsg(Client &client, const Args &args)
{
	std::string	command = upper(args[0]);
	bool		quiet = (command == "NOTICE");

	if (args.size() < 3 || args[2].empty())
	{
		if (!quiet && args.size() < 2)
			reply(client, "411", ":No recipient given");
		else if (!quiet)
			reply(client, "412", ":No text to send");
		return;
	}

	std::string message = ":" + client.prefix() + " " + command + " " + args[1] + " :" + args[2];

	if (args[1][0] == '#')
	{
		Channel *channel = findChannel(args[1]);
		if (channel != NULL && channel->has(client.fd))
			broadcast(*channel, message, client.fd);
		else if (!quiet && channel == NULL)
			reply(client, "403", args[1] + " :No such channel");
		else if (!quiet)
			reply(client, "404", args[1] + " :Cannot send to channel (join it first)");
	}
	else
	{
		Client *target = findNick(args[1]);
		if (target != NULL && target->registered)
			sendTo(*target, message);
		else if (!quiet)
			reply(client, "401", args[1] + " :No such nick");
	}
}

/* ********************** operator commands *********************** */

// TOPIC <#chan>            -> show the topic
// TOPIC <#chan> :<text>    -> change the topic
void	Server::cmdTopic(Client &client, const Args &args)
{
	if (args.size() < 2)
		return reply(client, "461", "TOPIC :Not enough parameters");

	Channel *channel = findChannel(args[1]);
	if (channel == NULL)
		return reply(client, "403", args[1] + " :No such channel");
	if (!channel->has(client.fd))
		return reply(client, "442", channel->name + " :You're not on that channel");

	if (args.size() == 2)
	{
		if (channel->topic.empty())
			return reply(client, "331", channel->name + " :No topic is set");
		return reply(client, "332", channel->name + " :" + channel->topic);
	}
	if (channel->topicLocked && !channel->isOperator(client.fd))
		return reply(client, "482", channel->name + " :You're not channel operator");

	channel->topic = args[2];
	broadcast(*channel, ":" + client.prefix() + " TOPIC " + channel->name + " :" + args[2], -1);
}

// KICK <#chan> <nick> [:reason]
void	Server::cmdKick(Client &client, const Args &args)
{
	if (args.size() < 3)
		return reply(client, "461", "KICK :Not enough parameters");

	Channel *channel = findChannel(args[1]);
	if (channel == NULL)
		return reply(client, "403", args[1] + " :No such channel");
	if (!channel->has(client.fd))
		return reply(client, "442", channel->name + " :You're not on that channel");
	if (!channel->isOperator(client.fd))
		return reply(client, "482", channel->name + " :You're not channel operator");

	Client *target = findNick(args[2]);
	if (target == NULL || !channel->has(target->fd))
		return reply(client, "441", args[2] + " " + channel->name + " :They aren't on that channel");

	std::string reason = args.size() > 3 ? args[3] : "Kicked";
	broadcast(*channel, ":" + client.prefix() + " KICK " + channel->name + " " + target->nick + " :" + reason, -1);
	leaveChannel(*channel, target->fd);
}

// INVITE <nick> <#chan>
void	Server::cmdInvite(Client &client, const Args &args)
{
	if (args.size() < 3)
		return reply(client, "461", "INVITE :Not enough parameters");

	Client	*target = findNick(args[1]);
	Channel	*channel = findChannel(args[2]);

	if (target == NULL || !target->registered)
		return reply(client, "401", args[1] + " :No such nick");
	if (channel == NULL)
		return reply(client, "403", args[2] + " :No such channel");
	if (!channel->has(client.fd))
		return reply(client, "442", channel->name + " :You're not on that channel");
	if (!channel->isOperator(client.fd))
		return reply(client, "482", channel->name + " :You're not channel operator");
	if (channel->has(target->fd))
		return reply(client, "443", target->nick + " " + channel->name + " :is already on channel");

	channel->invited.insert(target->fd);
	reply(client, "341", target->nick + " " + channel->name);
	sendTo(*target, ":" + client.prefix() + " INVITE " + target->nick + " " + channel->name);
}

// MODE <#chan>                      -> show the modes
// MODE <#chan> <+/-modes> [params]  -> change them, for example: MODE #42 +kl secret 10
//   i = invite only   t = topic locked   k = key   l = user limit   o = operator
void	Server::cmdMode(Client &client, const Args &args)
{
	if (args.size() < 2)
		return reply(client, "461", "MODE :Not enough parameters");
	if (args[1][0] != '#')		// "MODE <nick>": IRC clients send it, we have no user modes
		return reply(client, "221", "+");

	Channel *channel = findChannel(args[1]);
	if (channel == NULL)
		return reply(client, "403", args[1] + " :No such channel");
	if (args.size() == 2)
		return reply(client, "324", channel->name + " " + channel->modes());
	if (args[2] == "b" || args[2] == "+b")	// ban list asked by IRC clients: always empty
		return reply(client, "368", channel->name + " :End of channel ban list");
	if (!channel->isOperator(client.fd))
		return reply(client, "482", channel->name + " :You're not channel operator");

	bool	adding = true;
	size_t	next = 3;		// index of the next unused parameter

	for (size_t i = 0; i < args[2].size(); i++)
	{
		char		mode = args[2][i];
		std::string	param;

		if (mode == '+' || mode == '-')
		{
			adding = (mode == '+');
			continue;
		}
		// +k, +l, +o and -o need a parameter
		if (mode == 'o' || (adding && (mode == 'k' || mode == 'l')))
		{
			if (next >= args.size())
			{
				reply(client, "461", "MODE :Not enough parameters");
				continue;
			}
			param = args[next++];
		}

		if (mode == 'i')
			channel->inviteOnly = adding;
		else if (mode == 't')
			channel->topicLocked = adding;
		else if (mode == 'k')
			channel->key = param;			// param is empty when removing
		else if (mode == 'l')
		{
			int limit = std::atoi(param.c_str());
			if (adding && (limit <= 0 || param.size() > 6))
				continue;					// not a valid number: ignore it
			channel->limit = adding ? limit : 0;
		}
		else if (mode == 'o')
		{
			Client *target = findNick(param);
			if (target == NULL || !channel->has(target->fd))
			{
				reply(client, "441", param + " " + channel->name + " :They aren't on that channel");
				continue;
			}
			if (adding)
				channel->operators.insert(target->fd);
			else
				channel->operators.erase(target->fd);
		}
		else
		{
			reply(client, "472", std::string(1, mode) + " :is unknown mode char to me");
			continue;
		}

		// tell everybody in the channel what changed
		std::string change = std::string(adding ? "+" : "-") + mode;
		if (!param.empty())
			change += " " + param;
		broadcast(*channel, ":" + client.prefix() + " MODE " + channel->name + " " + change, -1);
	}
}

// WHO <#chan>   (IRC clients send it automatically after a JOIN)
void	Server::cmdWho(Client &client, const Args &args)
{
	std::string	name = args.size() > 1 ? args[1] : "*";
	Channel		*channel = findChannel(name);

	if (channel != NULL)
	{
		for (std::set<int>::iterator it = channel->members.begin(); it != channel->members.end(); ++it)
		{
			Client &member = _clients.find(*it)->second;
			reply(client, "352", channel->name + " " + member.user + " " + member.host + " " + SERVER_NAME
				+ " " + member.nick + (channel->isOperator(*it) ? " H@" : " H") + " :0 " + member.realname);
		}
	}
	reply(client, "315", name + " :End of WHO list");
}
