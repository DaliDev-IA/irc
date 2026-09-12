#include "Server.hpp"

#include <sstream>

void	Server::handle_topic(int fd, const ParsedCommand &command)
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

	if (command.params.empty())
	{
		std::ostringstream reply;
		reply << ":" << server_name() << " 461 " << client->get_nickname() << " TOPIC :Not enough parameters\r\n";
		queue_message(fd, reply.str());
		return;
	}

	Channel *channel = find_channel_by_name(command.params[0]);

	if (channel == NULL)
	{
		std::ostringstream reply;
		reply << ":" << server_name() << " 403 " << client->get_nickname() << " " << command.params[0] << " :No such channel\r\n";
		queue_message(fd, reply.str());
		return;
	}

	const std::string channelName = channel->get_name();
	bool changeTopic = command.hasTrailing || command.params.size() > 1;

	if (!changeTopic)
	{
		std::ostringstream reply;

		if (channel->has_topic())
			reply << ":" << server_name() << " 332 " << client->get_nickname() << " " << channelName << " :" << channel->get_topic() << "\r\n";
		else
			reply << ":" << server_name() << " 331 " << client->get_nickname() << " " << channelName << " :No topic is set\r\n";

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

	if (channel->is_topic_restricted() && !channel->is_operator(fd))
	{
		std::ostringstream reply;
		reply << ":" << server_name() << " 482 " << client->get_nickname() << " " << channelName << " :You're not channel operator\r\n";
		queue_message(fd, reply.str());
		return;
	}

	std::string newTopic;

	if (command.hasTrailing)
		newTopic = command.trailing;
	else
		newTopic = command.params[1];

	channel->set_topic(newTopic);

	std::string message = client_prefix(*client) + " TOPIC " + channelName + " :" + newTopic + "\r\n";
	broadcast_to_channel(*channel, message, -1);
}

void	Server::handle_kick(int fd, const ParsedCommand &command)
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
		reply << ":" << server_name() << " 461 " << client->get_nickname() << " KICK :Not enough parameters\r\n";
		queue_message(fd, reply.str());
		return;
	}

	Channel *channel = find_channel_by_name(command.params[0]);

	if (channel == NULL)
	{
		std::ostringstream reply;
		reply << ":" << server_name() << " 403 " << client->get_nickname() << " " << command.params[0] << " :No such channel\r\n";
		queue_message(fd, reply.str());
		return;
	}

	const std::string channelName = channel->get_name();
	const std::string &targetNick = command.params[1];

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

	Client *target = find_client_by_nickname(targetNick);

	if (target == NULL)
	{
		std::ostringstream reply;
		reply << ":" << server_name() << " 401 " << client->get_nickname() << " " << targetNick << " :No such nick\r\n";
		queue_message(fd, reply.str());
		return;
	}

	if (!channel->has_member(target->get_fd()))
	{
		std::ostringstream reply;
		reply << ":" << server_name() << " 441 " << client->get_nickname() << " " << target->get_nickname() << " " << channelName << " :They aren't on that channel\r\n";
		queue_message(fd, reply.str());
		return;
	}

	std::string reason = client->get_nickname();

	if (command.hasTrailing)
		reason = command.trailing;
	else if (command.params.size() > 2)
		reason = command.params[2];

	int targetFD = target->get_fd();

	std::string message = client_prefix(*client) + " KICK " + channelName + " " + target->get_nickname() + " :" + reason + "\r\n";
	broadcast_to_channel(*channel, message, -1);

	channel->remove_member(targetFD);
	remove_channel_if_empty(channelName);
}