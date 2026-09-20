#include "Server.hpp"

#include <sstream>

void	Server::send_channel_modes(int fd, const Channel &channel)
{
	Client *client = find_client_by_fd(fd);

	if (client == NULL)
		return;

	std::string modes = "+";
	std::string params;

	if (channel.is_invite_only())
		modes += "i";

	if (channel.is_topic_restricted())
		modes += "t";

	if (channel.has_key())
	{
		modes += "k";
		params += " " + channel.get_key();
	}

	if (channel.has_user_limit())
	{
		modes += "l";

		std::ostringstream limit;
		limit << channel.get_user_limit();
		params += " " + limit.str();
	}

	std::ostringstream reply;
	reply << ":" << server_name() << " 324 " << client->get_nickname() << " " << channel.get_name() << " " << modes << params << "\r\n";
	queue_message(fd, reply.str());
}

void	Server::handle_mode(int fd, const ParsedCommand &command)
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
		reply << ":" << server_name() << " 461 " << client->get_nickname() << " MODE :Not enough parameters\r\n";
		queue_message(fd, reply.str());
		return;
	}

	if (command.params[0][0] != '#')
	{
		handle_user_mode(fd, command);
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

	if (command.params.size() == 2 && (command.params[1] == "b" || command.params[1] == "+b"))
	{
		std::ostringstream reply;
		reply << ":" << server_name() << " 368 " << client->get_nickname() << " " << channel->get_name() << " :End of channel ban list\r\n";
		queue_message(fd, reply.str());
		return;
	}

	if (!channel->has_member(fd))
	{
		std::ostringstream reply;
		reply << ":" << server_name() << " 442 " << client->get_nickname() << " " << channel->get_name() << " :You're not on that channel\r\n";
		queue_message(fd, reply.str());
		return;
	}

	if (command.params.size() == 1)
	{
		send_channel_modes(fd, *channel);
		return;
	}

	if (!channel->is_operator(fd))
	{
		std::ostringstream reply;
		reply << ":" << server_name() << " 482 " << client->get_nickname() << " " << channel->get_name() << " :You're not channel operator\r\n";
		queue_message(fd, reply.str());
		return;
	}

	apply_channel_modes(fd, *channel, command);
}

void	Server::handle_user_mode(int fd, const ParsedCommand &command)
{
	Client *client = find_client_by_fd(fd);

	if (client == NULL)
		return;

	std::ostringstream reply;

	if (find_client_by_nickname(command.params[0]) == NULL)
		reply << ":" << server_name() << " 401 " << client->get_nickname() << " " << command.params[0] << " :No such nick\r\n";
	else if (!ascii_case_equal(command.params[0], client->get_nickname()))
		reply << ":" << server_name() << " 502 " << client->get_nickname() << " :Cannot change mode for other users\r\n";
	else
		reply << ":" << server_name() << " 221 " << client->get_nickname() << " +\r\n";

	queue_message(fd, reply.str());
}
