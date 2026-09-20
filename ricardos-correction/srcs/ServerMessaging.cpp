#include "Server.hpp"

#include <sstream>

void	Server::handle_privmsg(int fd, const ParsedCommand &command)
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
		reply << ":" << server_name() << " 411 " << client->get_nickname() << " :No recipient given (PRIVMSG)\r\n";
		queue_message(fd, reply.str());
		return;
	}

	std::string text;

	if (command.hasTrailing)
		text = command.trailing;
	else if (command.params.size() > 1)
		text = command.params[1];

	if (text.empty())
	{
		std::ostringstream reply;
		reply << ":" << server_name() << " 412 " << client->get_nickname() << " :No text to send\r\n";
		queue_message(fd, reply.str());
		return;
	}

	const std::string &target = command.params[0];

	if (!target.empty() && target[0] == '#')
	{
		Channel *channel = find_channel_by_name(target);

		if (channel == NULL)
		{
			std::ostringstream reply;
			reply << ":" << server_name() << " 403 " << client->get_nickname() << " " << target << " :No such channel\r\n";
			queue_message(fd, reply.str());
			return;
		}

		if (!channel->has_member(fd))
		{
			std::ostringstream reply;
			reply << ":" << server_name() << " 404 " << client->get_nickname() << " " << channel->get_name() << " :Cannot send to channel\r\n";
			queue_message(fd, reply.str());
			return;
		}

		std::string message = client_prefix(*client) + " PRIVMSG " + channel->get_name() + " :" + text + "\r\n";
		broadcast_to_channel(*channel, message, fd);
		return;
	}

	Client *targetClient = find_client_by_nickname(target);

	if (targetClient == NULL)
	{
		std::ostringstream reply;
		reply << ":" << server_name() << " 401 " << client->get_nickname() << " " << target << " :No such nick\r\n";
		queue_message(fd, reply.str());
		return;
	}

	std::string message = client_prefix(*client) + " PRIVMSG " + targetClient->get_nickname() + " :" + text + "\r\n";
	queue_message(targetClient->get_fd(), message);
}

void	Server::handle_notice(int fd, const ParsedCommand &command)
{
	Client *client = find_client_by_fd(fd);

	if (client == NULL || !client->is_registered() || command.params.empty())
		return;

	std::string text;

	if (command.hasTrailing)
		text = command.trailing;
	else if (command.params.size() > 1)
		text = command.params[1];

	if (text.empty())
		return;

	const std::string &target = command.params[0];

	if (target[0] == '#')
	{
		Channel *channel = find_channel_by_name(target);

		if (channel == NULL || !channel->has_member(fd))
			return;

		std::string message = client_prefix(*client) + " NOTICE " + channel->get_name() + " :" + text + "\r\n";
		broadcast_to_channel(*channel, message, fd);
		return;
	}

	Client *targetClient = find_client_by_nickname(target);

	if (targetClient == NULL)
		return;

	std::string message = client_prefix(*client) + " NOTICE " + targetClient->get_nickname() + " :" + text + "\r\n";
	queue_message(targetClient->get_fd(), message);
}
