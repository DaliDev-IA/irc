#include "Server.hpp"
#include <cstdlib>

// TOPIC <#chan>          -> show the topic
// TOPIC <#chan> :<text>  -> change it (only operators when mode +t)
void	Server::cmdTopic(Client &client, const Args &args)
{
	if (args.size() < 2)
		return (reply(client, "461", "TOPIC :Not enough parameters"));

	Channel	*chan = findChannel(args[1]);
	if (!chan)
		return (reply(client, "403", args[1] + " :No such channel"));
	if (!chan->has(client.fd))
		return (reply(client, "442", chan->name + " :You're not on that channel"));

	if (args.size() == 2)
	{
		if (chan->topic.empty())
			return (reply(client, "331", chan->name + " :No topic is set"));
		return (reply(client, "332", chan->name + " :" + chan->topic));
	}
	if (chan->topicOps && !chan->isOp(client.fd))
		return (reply(client, "482", chan->name + " :You're not channel operator"));
	chan->topic = args[2];
	broadcast(*chan, ":" + client.prefix() + " TOPIC " + chan->name + " :" + chan->topic, -1);
}

// KICK <#chan> <nick> [:reason]
void	Server::cmdKick(Client &client, const Args &args)
{
	if (args.size() < 3)
		return (reply(client, "461", "KICK :Not enough parameters"));

	Channel	*chan = findChannel(args[1]);
	if (!chan)
		return (reply(client, "403", args[1] + " :No such channel"));
	if (!chan->has(client.fd))
		return (reply(client, "442", chan->name + " :You're not on that channel"));
	if (!chan->isOp(client.fd))
		return (reply(client, "482", chan->name + " :You're not channel operator"));

	Client	*target = findNick(args[2]);
	if (!target || !chan->has(target->fd))
		return (reply(client, "441", args[2] + " " + chan->name + " :They aren't on that channel"));

	std::string	reason = args.size() > 3 ? args[3] : client.nick;
	broadcast(*chan, ":" + client.prefix() + " KICK " + chan->name + " " + target->nick + " :" + reason, -1);
	leaveChannel(*chan, target->fd);
}

// INVITE <nick> <#chan>
void	Server::cmdInvite(Client &client, const Args &args)
{
	if (args.size() < 3)
		return (reply(client, "461", "INVITE :Not enough parameters"));

	Client	*target = findNick(args[1]);
	Channel	*chan = findChannel(args[2]);

	if (!target || !target->registered)
		return (reply(client, "401", args[1] + " :No such nick"));
	if (!chan)
		return (reply(client, "403", args[2] + " :No such channel"));
	if (!chan->has(client.fd))
		return (reply(client, "442", chan->name + " :You're not on that channel"));
	if (!chan->isOp(client.fd))
		return (reply(client, "482", chan->name + " :You're not channel operator"));
	if (chan->has(target->fd))
		return (reply(client, "443", target->nick + " " + chan->name + " :is already on channel"));

	chan->invited.insert(target->fd);
	reply(client, "341", target->nick + " " + chan->name);
	sendTo(*target, ":" + client.prefix() + " INVITE " + target->nick + " " + chan->name);
}

// MODE <#chan>                     -> show the modes
// MODE <#chan> <+/-modes> [params] -> change them, ex: MODE #42 +kl secret 10
//   i: invite only   t: topic for operators only   k: key   l: user limit   o: operator
void	Server::cmdMode(Client &client, const Args &args)
{
	if (args.size() < 2)
		return (reply(client, "461", "MODE :Not enough parameters"));
	if (args[1][0] != '#')
		return ;	// "MODE <nick>" sent by irssi: we have no user modes

	Channel	*chan = findChannel(args[1]);
	if (!chan)
		return (reply(client, "403", args[1] + " :No such channel"));
	if (args.size() == 2)
		return (reply(client, "324", chan->name + " " + chan->modes()));
	if (args[2] == "b")
		return (reply(client, "368", chan->name + " :End of channel ban list"));	// irssi asks it on join
	if (!chan->isOp(client.fd))
		return (reply(client, "482", chan->name + " :You're not channel operator"));

	bool	adding = true;
	size_t	next = 3;	// index of the next parameter to use

	for (size_t i = 0; i < args[2].size(); i++)
	{
		char		mode = args[2][i];
		std::string	param;

		if (mode == '+' || mode == '-')
		{
			adding = (mode == '+');
			continue ;
		}
		if (mode == 'o' || (adding && (mode == 'k' || mode == 'l')))	// these need a parameter
		{
			if (next >= args.size())
			{
				reply(client, "461", "MODE :Not enough parameters");
				continue ;
			}
			param = args[next++];
		}

		if (mode == 'i')
			chan->inviteOnly = adding;
		else if (mode == 't')
			chan->topicOps = adding;
		else if (mode == 'k')
			chan->key = param;	// empty when removing
		else if (mode == 'l')
		{
			if (adding && (!isNumber(param) || param.size() > 6 || std::atoi(param.c_str()) == 0))
				continue ;	// not a valid limit, ignore it
			chan->limit = adding ? std::atoi(param.c_str()) : 0;
		}
		else if (mode == 'o')
		{
			Client	*target = findNick(param);
			if (!target || !chan->has(target->fd))
			{
				reply(client, "441", param + " " + chan->name + " :They aren't on that channel");
				continue ;
			}
			if (adding)
				chan->ops.insert(target->fd);
			else
				chan->ops.erase(target->fd);
		}
		else
		{
			reply(client, "472", std::string(1, mode) + " :is unknown mode char to me");
			continue ;
		}

		std::string	change = std::string(adding ? "+" : "-") + mode;
		if (!param.empty())
			change += " " + param;
		broadcast(*chan, ":" + client.prefix() + " MODE " + chan->name + " " + change, -1);
	}
}
