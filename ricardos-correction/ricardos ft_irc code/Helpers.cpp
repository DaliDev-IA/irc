#include "Server.hpp"

// Nothing is sent right away: the message waits in the outbox until poll()
// tells us the socket is ready for writing.
void	Server::sendTo(Client &client, const std::string &message)
{
	if (!client.dead)
		client.outbox += message + "\r\n";
}

// A numeric reply from the server, for example:
//   ":ircserv 461 bob JOIN :Not enough parameters"
void	Server::reply(Client &client, const std::string &code, const std::string &text)
{
	std::string nick = client.nick.empty() ? "*" : client.nick;
	sendTo(client, std::string(":") + SERVER_NAME + " " + code + " " + nick + " " + text);
}

// Send a message to everybody in a channel (exceptFd = -1 means "no exception").
void	Server::broadcast(Channel &channel, const std::string &message, int exceptFd)
{
	for (std::set<int>::iterator it = channel.members.begin(); it != channel.members.end(); ++it)
		if (*it != exceptFd)
			sendTo(_clients.find(*it)->second, message);
}

// Nicknames and channel names are not case sensitive: Bob == bob.
Client	*Server::findNick(const std::string &nick)
{
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		if (!it->second.dead && lower(it->second.nick) == lower(nick))
			return &it->second;
	return NULL;
}

Channel	*Server::findChannel(const std::string &name)
{
	std::map<std::string, Channel>::iterator it = _channels.find(lower(name));

	if (it == _channels.end())
		return NULL;
	return &it->second;
}

// Everybody who shares at least one channel with this client.
std::set<int>	Server::neighbours(Client &client)
{
	std::set<int> result;

	for (std::map<std::string, Channel>::iterator it = _channels.begin(); it != _channels.end(); ++it)
		if (it->second.has(client.fd))
			result.insert(it->second.members.begin(), it->second.members.end());
	result.erase(client.fd);
	return result;
}

// Remove somebody from a channel. An empty channel is deleted.
void	Server::leaveChannel(Channel &channel, int fd)
{
	channel.remove(fd);
	if (channel.members.empty())
		_channels.erase(lower(channel.name));
}

// Used for QUIT and for lost connections: tell the others, leave every
// channel, and mark the client so the main loop closes it.
void	Server::disconnect(Client &client, const std::string &reason)
{
	if (client.dead)
		return;

	std::set<int> others = neighbours(client);
	for (std::set<int>::iterator it = others.begin(); it != others.end(); ++it)
		sendTo(_clients.find(*it)->second, ":" + client.prefix() + " QUIT :" + reason);

	std::map<std::string, Channel>::iterator it = _channels.begin();
	while (it != _channels.end())
	{
		Channel &channel = (it++)->second;	// move on first: the channel may be deleted
		leaveChannel(channel, client.fd);
	}
	client.dead = true;
}

// A client is registered once it gave the password, a nickname and a username.
void	Server::checkRegistration(Client &client)
{
	if (client.registered || client.nick.empty() || client.user.empty())
		return;
	if (!client.passOk)
	{
		reply(client, "464", ":Password required (use PASS first)");
		return;
	}
	client.registered = true;
	reply(client, "001", ":Welcome to the IRC network " + client.prefix());
	reply(client, "002", std::string(":Your host is ") + SERVER_NAME);
	reply(client, "003", ":This server was created for ft_irc");
	reply(client, "004", std::string(SERVER_NAME) + " 1.0 o itkol");
	reply(client, "375", ":- Message of the day -");
	reply(client, "372", ":- Be nice and have fun!");
	reply(client, "376", ":End of /MOTD command");
}
