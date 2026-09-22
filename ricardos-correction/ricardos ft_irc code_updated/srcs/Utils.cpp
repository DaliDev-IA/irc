#include "Server.hpp"
#include <cctype>

/* ---------------- string helpers ---------------- */

std::string	lower(std::string s)
{
	for (size_t i = 0; i < s.size(); i++)
		s[i] = std::tolower(s[i]);
	return (s);
}

std::string	upper(std::string s)
{
	for (size_t i = 0; i < s.size(); i++)
		s[i] = std::toupper(s[i]);
	return (s);
}

bool	isNumber(const std::string &s)
{
	if (s.empty())
		return (false);
	for (size_t i = 0; i < s.size(); i++)
		if (!std::isdigit(s[i]))
			return (false);
	return (true);
}

// split("a,b,c", ',') -> ["a", "b", "c"]
Args	split(const std::string &s, char sep)
{
	Args	parts;
	size_t	start = 0;
	size_t	end;

	while ((end = s.find(sep, start)) != std::string::npos)
	{
		if (end > start)
			parts.push_back(s.substr(start, end - start));
		start = end + 1;
	}
	if (start < s.size())
		parts.push_back(s.substr(start));
	return (parts);
}

// "KICK #chan bob :see you" -> ["KICK", "#chan", "bob", "see you"]
// a word starting with ':' is the last argument and can contain spaces
Args	parseLine(const std::string &line)
{
	Args	args;
	size_t	i = 0;
	size_t	end;

	while (i < line.size())
	{
		if (line[i] == ' ')
		{
			i++;
			continue ;
		}
		if (line[i] == ':' && !args.empty())
		{
			args.push_back(line.substr(i + 1));
			break ;
		}
		end = line.find(' ', i);
		if (end == std::string::npos)
			end = line.size();
		args.push_back(line.substr(i, end - i));
		i = end;
	}
	if (!args.empty() && args[0][0] == ':')	// ":prefix CMD ..." we don't need the prefix
		args.erase(args.begin());
	return (args);
}

/* ---------------- server helpers ---------------- */

// nothing is sent directly: it waits in outbuf until poll() says we can write
void	Server::sendTo(Client &client, const std::string &msg)
{
	if (!client.dead)
		client.outbuf += msg + "\r\n";
}

// numeric reply, ex: ":ircserv 461 bob JOIN :Not enough parameters"
void	Server::reply(Client &client, const std::string &code, const std::string &text)
{
	std::string	nick = client.nick.empty() ? "*" : client.nick;

	sendTo(client, ":" SERVER_NAME " " + code + " " + nick + " " + text);
}

void	Server::broadcast(Channel &chan, const std::string &msg, int exceptFd)
{
	for (std::set<int>::iterator it = chan.members.begin(); it != chan.members.end(); ++it)
		if (*it != exceptFd)
			sendTo(_clients.find(*it)->second, msg);
}

// nicks and channel names are case insensitive
Client	*Server::findNick(const std::string &nick)
{
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		if (!it->second.dead && lower(it->second.nick) == lower(nick))
			return (&it->second);
	return (NULL);
}

Channel	*Server::findChannel(const std::string &name)
{
	std::map<std::string, Channel>::iterator	it = _channels.find(lower(name));

	if (it == _channels.end())
		return (NULL);
	return (&it->second);
}

// everybody who shares a channel with this client (without the client itself)
std::set<int>	Server::neighbours(Client &client)
{
	std::set<int>	result;

	for (std::map<std::string, Channel>::iterator it = _channels.begin(); it != _channels.end(); ++it)
		if (it->second.has(client.fd))
			result.insert(it->second.members.begin(), it->second.members.end());
	result.erase(client.fd);
	return (result);
}

// an empty channel is deleted, so don't use chan after this call
void	Server::leaveChannel(Channel &chan, int fd)
{
	chan.members.erase(fd);
	chan.ops.erase(fd);
	chan.invited.erase(fd);
	if (chan.members.empty())
		_channels.erase(lower(chan.name));
}

// for QUIT and lost connections: warn the others, leave every channel,
// and mark the client so the main loop closes its socket
void	Server::disconnect(Client &client, const std::string &reason)
{
	if (client.dead)
		return ;

	std::set<int>	others = neighbours(client);
	for (std::set<int>::iterator it = others.begin(); it != others.end(); ++it)
		sendTo(_clients.find(*it)->second, ":" + client.prefix() + " QUIT :" + reason);

	std::map<std::string, Channel>::iterator	it = _channels.begin();
	while (it != _channels.end())
	{
		Channel	&chan = (it++)->second;	// step first, leaveChannel may erase it
		if (chan.has(client.fd))
			leaveChannel(chan, client.fd);
	}
	client.dead = true;
}

// registered = correct PASS + NICK + USER
void	Server::tryRegister(Client &client)
{
	if (client.registered || client.nick.empty() || client.user.empty())
		return ;
	if (!client.passOk)
	{
		reply(client, "464", ":Password required");
		return ;
	}
	client.registered = true;
	reply(client, "001", ":Welcome to the IRC network " + client.prefix());
	reply(client, "002", ":Your host is " SERVER_NAME);
	reply(client, "003", ":This server was created for ft_irc");
	reply(client, "004", SERVER_NAME " 1.0 o itkol");
	reply(client, "422", ":MOTD File is missing");
}
