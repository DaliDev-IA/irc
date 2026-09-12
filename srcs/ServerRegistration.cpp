#include "Server.hpp"

#include <sstream>

void	Server::try_register_client(int fd)
{
	Client *client = find_client_by_fd(fd);

	if (client == NULL || client->is_registered())
		return;

	if (!client->is_password_accepted())
		return;

	if (client->get_nickname().empty() || client->get_username().empty())
		return;

	if (client->is_cap_negotiating())
		return;

	client->set_registered(true);

	std::ostringstream reply;

	reply << ":" << server_name() << " 001 " << client->get_nickname() << " :Welcome to the Internet Relay Network " << client->get_nickname() << "!" << client->get_username() << "@" << client->get_ip() << "\r\n";
	reply << ":" << server_name() << " 002 " << client->get_nickname() << " :Your host is " << server_name() << ", running version ft_irc-1.0\r\n";
	reply << ":" << server_name() << " 003 " << client->get_nickname() << " :This server was created for ft_irc\r\n";
	reply << ":" << server_name() << " 004 " << client->get_nickname() << " " << server_name() << " ft_irc-1.0 - itkol\r\n";
	reply << ":" << server_name() << " 422 " << client->get_nickname() << " :MOTD File is missing\r\n";

	queue_message(fd, reply.str());
}

void	Server::handle_pass(int fd, const ParsedCommand &command)
{
	Client *client = find_client_by_fd(fd);

	if (client == NULL)
		return;

	if (client->is_registered())
	{
		std::ostringstream reply;
		reply << ":" << server_name() << " 462 " << client_target(*client) << " :You may not reregister\r\n";
		queue_message(fd, reply.str());
		return;
	}

	if (command.params.empty())
	{
		client->set_password_accepted(false);

		std::ostringstream reply;
		reply << ":" << server_name() << " 461 " << client_target(*client) << " PASS :Not enough parameters\r\n";
		queue_message(fd, reply.str());
		return;
	}

	if (command.params[0] != _password)
	{
		client->set_password_accepted(false);

		std::ostringstream reply;
		reply << ":" << server_name() << " 464 " << client_target(*client) << " :Password incorrect\r\n";
		queue_message(fd, reply.str());
		return;
	}

	client->set_password_accepted(true);
	try_register_client(fd);
}

void	Server::handle_nick(int fd, const ParsedCommand &command)
{
	Client *client = find_client_by_fd(fd);

	if (client == NULL)
		return;

	if (command.params.empty())
	{
		std::ostringstream reply;
		reply << ":" << server_name() << " 431 " << client_target(*client) << " :No nickname given\r\n";
		queue_message(fd, reply.str());
		return;
	}

	const std::string &nickname = command.params[0];

	if (!is_nickname_valid(nickname))
	{
		std::ostringstream reply;
		reply << ":" << server_name() << " 432 " << client_target(*client) << " " << nickname << " :Erroneous nickname\r\n";
		queue_message(fd, reply.str());
		return;
	}

	if (!is_nickname_available(nickname, fd))
	{
		std::ostringstream reply;
		reply << ":" << server_name() << " 433 " << client_target(*client) << " " << nickname << " :Nickname is already in use\r\n";
		queue_message(fd, reply.str());
		return;
	}

	client->set_nickname(nickname);
	try_register_client(fd);
}

void	Server::handle_user(int fd, const ParsedCommand &command)
{
	Client *client = find_client_by_fd(fd);

	if (client == NULL)
		return;

	if (client->is_registered())
	{
		std::ostringstream reply;
		reply << ":" << server_name() << " 462 " << client_target(*client) << " :You may not reregister\r\n";
		queue_message(fd, reply.str());
		return;
	}

	if (command.params.size() < 3 || (!command.hasTrailing && command.params.size() < 4))
	{
		std::ostringstream reply;
		reply << ":" << server_name() << " 461 " << client_target(*client) << " USER :Not enough parameters\r\n";
		queue_message(fd, reply.str());
		return;
	}

	client->set_username(command.params[0]);

	if (command.hasTrailing)
		client->set_realname(command.trailing);
	else
		client->set_realname(command.params[3]);

	try_register_client(fd);
}