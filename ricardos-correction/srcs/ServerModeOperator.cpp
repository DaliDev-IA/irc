#include "Server.hpp"

#include <sstream>

bool	Server::apply_operator_mode(int fd, Channel &channel, bool adding, const ParsedCommand &command, std::vector<std::string>::size_type &paramIndex, std::string &appliedParam)
{
	Client *client = find_client_by_fd(fd);

	if (client == NULL)
		return (false);

	if (paramIndex >= command.params.size())
	{
		std::ostringstream reply;
		reply << ":" << server_name() << " 461 " << client->get_nickname() << " MODE :Not enough parameters\r\n";
		queue_message(fd, reply.str());
		return (false);
	}

	const std::string &targetNick = command.params[paramIndex];
	paramIndex++;

	Client *target = find_client_by_nickname(targetNick);

	if (target == NULL)
	{
		std::ostringstream reply;
		reply << ":" << server_name() << " 401 " << client->get_nickname() << " " << targetNick << " :No such nick\r\n";
		queue_message(fd, reply.str());
		return (false);
	}

	int targetFD = target->get_fd();

	if (!channel.has_member(targetFD))
	{
		std::ostringstream reply;
		reply << ":" << server_name() << " 441 " << client->get_nickname() << " " << target->get_nickname() << " " << channel.get_name() << " :They aren't on that channel\r\n";
		queue_message(fd, reply.str());
		return (false);
	}

	if (adding)
	{
		if (channel.is_operator(targetFD))
			return (false);
		channel.add_operator(targetFD);
	}
	else
	{
		if (!channel.is_operator(targetFD))
			return (false);
		channel.remove_operator(targetFD);
	}

	appliedParam = target->get_nickname();
	return (true);
}