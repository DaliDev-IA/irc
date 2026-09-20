#include "Server.hpp"

#include <sstream>

void	Server::handle_join(int fd, const ParsedCommand &command)
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
		reply << ":" << server_name() << " 461 " << client->get_nickname() << " JOIN :Not enough parameters\r\n";
		queue_message(fd, reply.str());
		return;
	}

	const std::string &requestedName = command.params[0];

	if (!is_channel_name_valid(requestedName))
	{
		std::ostringstream reply;
		reply << ":" << server_name() << " 476 " << client->get_nickname() << " " << requestedName << " :Bad Channel Mask\r\n";
		queue_message(fd, reply.str());
		return;
	}

	Channel *channel = find_channel_by_name(requestedName);
	bool created = false;

	if (channel == NULL)
	{
		_channels.push_back(Channel(requestedName));
		channel = &_channels.back();
		created = true;
	}

	const std::string channelName = channel->get_name();

	if (channel->has_member(fd))
		return;

	if (channel->is_invite_only() && !channel->is_invited(fd))
	{
		std::ostringstream reply;
		reply << ":" << server_name() << " 473 " << client->get_nickname() << " " << channelName << " :Cannot join channel (+i)\r\n";
		queue_message(fd, reply.str());
		return;
	}

	if (channel->has_key())
	{
		std::string providedKey;

		if (command.params.size() > 1)
			providedKey = command.params[1];

		if (providedKey != channel->get_key())
		{
			std::ostringstream reply;
			reply << ":" << server_name() << " 475 " << client->get_nickname() << " " << channelName << " :Cannot join channel (+k)\r\n";
			queue_message(fd, reply.str());
			return;
		}
	}

	if (channel->has_user_limit() && channel->get_members().size() >= static_cast<std::vector<int>::size_type>(channel->get_user_limit()))
	{
		std::ostringstream reply;
		reply << ":" << server_name() << " 471 " << client->get_nickname() << " " << channelName << " :Cannot join channel (+l)\r\n";
		queue_message(fd, reply.str());
		return;
	}

	channel->add_member(fd);
	channel->remove_invited(fd);

	if (created)
		channel->add_operator(fd);

	std::string message = client_prefix(*client) + " JOIN :" + channelName + "\r\n";
	broadcast_to_channel(*channel, message, -1);

	std::ostringstream reply;

	if (channel->has_topic())
		reply << ":" << server_name() << " 332 " << client->get_nickname() << " " << channelName << " :" << channel->get_topic() << "\r\n";
	else
		reply << ":" << server_name() << " 331 " << client->get_nickname() << " " << channelName << " :No topic is set\r\n";

	queue_message(fd, reply.str());
	send_names_reply(fd, *channel);
}

void	Server::handle_part(int fd, const ParsedCommand &command)
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
		reply << ":" << server_name() << " 461 " << client->get_nickname() << " PART :Not enough parameters\r\n";
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

	if (!channel->has_member(fd))
	{
		std::ostringstream reply;
		reply << ":" << server_name() << " 442 " << client->get_nickname() << " " << channelName << " :You're not on that channel\r\n";
		queue_message(fd, reply.str());
		return;
	}

	std::string reason;

	if (command.hasTrailing)
		reason = command.trailing;
	else if (command.params.size() > 1)
		reason = command.params[1];

	std::string message = client_prefix(*client) + " PART " + channelName;

	if (!reason.empty())
		message += " :" + reason;

	message += "\r\n";

	broadcast_to_channel(*channel, message, -1);
	channel->remove_member(fd);
	remove_channel_if_empty(channelName);
}