#include "Server.hpp"

#include <sstream>

void	Server::handle_invite(int fd, const ParsedCommand &command)
{
	Client *client = find_client_by_fd(fd);

	if (client == NULL)
		return;

	if (!client->is_registered())
	{
		std::ostringstream reply;
		reply << ":" << server_name() << " 451 " << client_target(*client) << " :You have not registered\r\n";
		queue_message(fd, reply.str());
		return;
	}

	if (command.params.size() < 2)
	{
		std::ostringstream reply;
		reply << ":" << server_name() << " 461 " << client->get_nickname() << " INVITE :Not enough parameters\r\n";
		queue_message(fd, reply.str());
		return;
	}

	const std::string &targetNick = command.params[0];
	const std::string &channelName = command.params[1];

	Client *target = find_client_by_nickname(targetNick);

	if (target == NULL)
	{
		std::ostringstream reply;
		reply << ":" << server_name() << " 401 " << client->get_nickname() << " " << targetNick << " :No such nick\r\n";
		queue_message(fd, reply.str());
		return;
	}

	Channel *channel = find_channel_by_name(channelName);

	if (channel == NULL)
	{
		std::ostringstream reply;
		reply << ":" << server_name() << " 403 " << client->get_nickname() << " " << channelName << " :No such channel\r\n";
		queue_message(fd, reply.str());
		return;
	}

	if (!channel->has_member(fd))
	{
		std::ostringstream reply;
		reply << ":" << server_name() << " 442 " << client->get_nickname() << " " << channelName << " :You're not on that channel\r\n";
		queue_message(fd, reply.str());
		return;
	}

	if (!channel->is_operator(fd))
	{
		std::ostringstream reply;
		reply << ":" << server_name() << " 482 " << client->get_nickname() << " " << channelName << " :You're not channel operator\r\n";
		queue_message(fd, reply.str());
		return;
	}

	if (channel->has_member(target->get_fd()))
	{
		std::ostringstream reply;
		reply << ":" << server_name() << " 443 " << client->get_nickname() << " " << target->get_nickname() << " " << channelName << " :is already on channel\r\n";
		queue_message(fd, reply.str());
		return;
	}

	channel->add_invited(target->get_fd());

	std::ostringstream reply;
	reply << ":" << server_name() << " 341 " << client->get_nickname() << " " << target->get_nickname() << " " << channel->get_name() << "\r\n";
	queue_message(fd, reply.str());

	std::string message = client_prefix(*client) + " INVITE " + target->get_nickname() + " :" + channel->get_name() + "\r\n";
	queue_message(target->get_fd(), message);
}