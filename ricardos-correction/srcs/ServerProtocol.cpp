#include "Server.hpp"

#include <sstream>

void	Server::handle_cap(int fd, const ParsedCommand &command)
{
	Client *client = find_client_by_fd(fd);

	if (client == NULL || command.params.empty())
		return;

	const std::string &subCommand = command.params[0];

	if (ascii_case_equal(subCommand, "LS"))
	{
		client->set_cap_negotiating(true);
		queue_message(fd, ":" + std::string(server_name()) + " CAP " + client_target(*client) + " LS :\r\n");
		return;
	}

	if (ascii_case_equal(subCommand, "REQ"))
	{
		client->set_cap_negotiating(true);

		std::string requested;

		if (command.hasTrailing)
			requested = command.trailing;
		else
		{
			std::string::size_type i = 1;

			while (i < command.params.size())
			{
				if (!requested.empty())
					requested += " ";
				requested += command.params[i];
				i++;
			}
		}

		queue_message(fd, ":" + std::string(server_name()) + " CAP " + client_target(*client) + " NAK :" + requested + "\r\n");
		return;
	}

	if (ascii_case_equal(subCommand, "LIST"))
	{
		queue_message(fd, ":" + std::string(server_name()) + " CAP " + client_target(*client) + " LIST :\r\n");
		return;
	}

	if (ascii_case_equal(subCommand, "END"))
	{
		client->set_cap_negotiating(false);
		try_register_client(fd);
	}
}

void	Server::handle_ping(int fd, const ParsedCommand &command)
{
	Client *client = find_client_by_fd(fd);

	if (client == NULL)
		return;

	std::string token;

	if (command.hasTrailing)
		token = command.trailing;
	else if (!command.params.empty())
		token = command.params[0];

	if (token.empty())
	{
		std::ostringstream reply;
		reply << ":" << server_name() << " 409 " << client_target(*client) << " :No origin specified\r\n";
		queue_message(fd, reply.str());
		return;
	}

	queue_message(fd, "PONG :" + token + "\r\n");
}

void	Server::send_unknown_command(int fd, const std::string &name)
{
	Client *client = find_client_by_fd(fd);

	if (client == NULL)
		return;

	std::ostringstream reply;

	if (!client->is_registered())
		reply << ":" << server_name() << " 451 " << client_target(*client) << " :You have not registered\r\n";
	else
		reply << ":" << server_name() << " 421 " << client->get_nickname() << " " << name << " :Unknown command\r\n";

	queue_message(fd, reply.str());
}
