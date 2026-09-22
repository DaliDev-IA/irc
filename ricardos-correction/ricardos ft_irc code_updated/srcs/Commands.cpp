#include "Server.hpp"

// called for every complete line received from a client
void	Server::handleLine(Client &client, const std::string &line)
{
	Args		args = parseLine(line);
	std::string	cmd;

	if (args.empty())
		return ;
	cmd = upper(args[0]);
	if (cmd == "CAP" || cmd == "PONG")
		return ;	// sent by IRC clients, nothing to do with them
	else if (cmd == "PASS")
		cmdPass(client, args);
	else if (cmd == "NICK")
		cmdNick(client, args);
	else if (cmd == "USER")
		cmdUser(client, args);
	else if (cmd == "QUIT")
		cmdQuit(client, args);
	else if (!client.registered)
		reply(client, "451", ":You have not registered");
	else if (cmd == "PING")
		cmdPing(client, args);
	else if (cmd == "JOIN")
		cmdJoin(client, args);
	else if (cmd == "PART")
		cmdPart(client, args);
	else if (cmd == "PRIVMSG")
		cmdPrivmsg(client, args);
	else if (cmd == "TOPIC")
		cmdTopic(client, args);
	else if (cmd == "KICK")
		cmdKick(client, args);
	else if (cmd == "INVITE")
		cmdInvite(client, args);
	else if (cmd == "MODE")
		cmdMode(client, args);
	else if (cmd == "WHO")
		reply(client, "315", (args.size() > 1 ? args[1] : "*") + " :End of WHO list");
	else
		reply(client, "421", cmd + " :Unknown command");
}

// PASS <password>
void	Server::cmdPass(Client &client, const Args &args)
{
	if (client.registered)
		return (reply(client, "462", ":You may not reregister"));
	if (args.size() < 2)
		return (reply(client, "461", "PASS :Not enough parameters"));
	if (args[1] != _password)
	{
		client.passOk = false;
		return (reply(client, "464", ":Password incorrect"));
	}
	client.passOk = true;
	tryRegister(client);
}

static bool	validNick(const std::string &nick)
{
	const std::string	special = "[]\\`_^{|}";

	if (nick.empty() || nick.size() > 30)
		return (false);
	for (size_t i = 0; i < nick.size(); i++)
	{
		if (isalpha(nick[i]) || special.find(nick[i]) != std::string::npos)
			continue ;
		if (i > 0 && (isdigit(nick[i]) || nick[i] == '-'))
			continue ;
		return (false);
	}
	return (true);
}

// NICK <nickname>
void	Server::cmdNick(Client &client, const Args &args)
{
	if (args.size() < 2)
		return (reply(client, "431", ":No nickname given"));
	if (!validNick(args[1]))
		return (reply(client, "432", args[1] + " :Erroneous nickname"));

	Client	*other = findNick(args[1]);
	if (other && other != &client)
		return (reply(client, "433", args[1] + " :Nickname is already in use"));

	if (client.registered)	// tell the user and everybody who can see him
	{
		std::string		msg = ":" + client.prefix() + " NICK :" + args[1];
		std::set<int>	others = neighbours(client);

		sendTo(client, msg);
		for (std::set<int>::iterator it = others.begin(); it != others.end(); ++it)
			sendTo(_clients.find(*it)->second, msg);
	}
	client.nick = args[1];
	tryRegister(client);
}

// USER <username> 0 * :<realname>
void	Server::cmdUser(Client &client, const Args &args)
{
	if (client.registered)
		return (reply(client, "462", ":You may not reregister"));
	if (args.size() < 5)
		return (reply(client, "461", "USER :Not enough parameters"));
	client.user = args[1];
	tryRegister(client);
}

// PING <token>
void	Server::cmdPing(Client &client, const Args &args)
{
	if (args.size() < 2)
		return (reply(client, "409", ":No origin specified"));
	sendTo(client, ":" SERVER_NAME " PONG " SERVER_NAME " :" + args[1]);
}

// QUIT [:reason]
void	Server::cmdQuit(Client &client, const Args &args)
{
	disconnect(client, args.size() > 1 ? args[1] : "Leaving");
}

// JOIN <#chan>[,<#chan>] [<key>[,<key>]]
void	Server::cmdJoin(Client &client, const Args &args)
{
	if (args.size() < 2)
		return (reply(client, "461", "JOIN :Not enough parameters"));

	Args	names = split(args[1], ',');
	Args	keys;

	if (args.size() > 2)
		keys = split(args[2], ',');
	for (size_t i = 0; i < names.size(); i++)
		joinOne(client, names[i], i < keys.size() ? keys[i] : "");
}

void	Server::joinOne(Client &client, const std::string &name, const std::string &key)
{
	if (name.size() < 2 || name.size() > 50 || name[0] != '#')
		return (reply(client, "476", name + " :Bad Channel Mask"));

	Channel	&chan = _channels[lower(name)];	// creates it when it does not exist
	bool	isNew = chan.members.empty();

	if (isNew)
		chan.name = name;
	if (chan.has(client.fd))
		return ;
	if (chan.inviteOnly && !chan.invited.count(client.fd))
		return (reply(client, "473", chan.name + " :Cannot join channel (+i)"));
	if (!chan.key.empty() && key != chan.key)
		return (reply(client, "475", chan.name + " :Cannot join channel (+k)"));
	if (chan.limit > 0 && (int)chan.members.size() >= chan.limit)
		return (reply(client, "471", chan.name + " :Cannot join channel (+l)"));

	chan.members.insert(client.fd);
	chan.invited.erase(client.fd);
	if (isNew)
		chan.ops.insert(client.fd);	// the creator is operator
	broadcast(chan, ":" + client.prefix() + " JOIN " + chan.name, -1);
	if (!chan.topic.empty())
		reply(client, "332", chan.name + " :" + chan.topic);

	std::string	names;
	for (std::set<int>::iterator it = chan.members.begin(); it != chan.members.end(); ++it)
		names += (chan.isOp(*it) ? "@" : "") + _clients.find(*it)->second.nick + " ";
	reply(client, "353", "= " + chan.name + " :" + names);
	reply(client, "366", chan.name + " :End of /NAMES list");
}

// PART <#chan> [:reason]
void	Server::cmdPart(Client &client, const Args &args)
{
	if (args.size() < 2)
		return (reply(client, "461", "PART :Not enough parameters"));

	Channel	*chan = findChannel(args[1]);
	if (!chan)
		return (reply(client, "403", args[1] + " :No such channel"));
	if (!chan->has(client.fd))
		return (reply(client, "442", chan->name + " :You're not on that channel"));

	std::string	reason = args.size() > 2 ? " :" + args[2] : "";
	broadcast(*chan, ":" + client.prefix() + " PART " + chan->name + reason, -1);
	leaveChannel(*chan, client.fd);
}

// PRIVMSG <nick or #chan> :<text>
void	Server::cmdPrivmsg(Client &client, const Args &args)
{
	if (args.size() < 2)
		return (reply(client, "411", ":No recipient given (PRIVMSG)"));
	if (args.size() < 3 || args[2].empty())
		return (reply(client, "412", ":No text to send"));

	std::string	msg = ":" + client.prefix() + " PRIVMSG " + args[1] + " :" + args[2];

	if (args[1][0] == '#')
	{
		Channel	*chan = findChannel(args[1]);
		if (!chan)
			return (reply(client, "403", args[1] + " :No such channel"));
		if (!chan->has(client.fd))
			return (reply(client, "404", args[1] + " :Cannot send to channel"));
		broadcast(*chan, msg, client.fd);
	}
	else
	{
		Client	*target = findNick(args[1]);
		if (!target || !target->registered)
			return (reply(client, "401", args[1] + " :No such nick"));
		sendTo(*target, msg);
	}
}
