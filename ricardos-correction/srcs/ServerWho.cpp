#include "Server.hpp"

#include <sstream>

void	Server::handle_who(int fd, const ParsedCommand &command)
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

	std::string mask = "*";

	if (!command.params.empty())
		mask = command.params[0];

	std::ostringstream reply;
	Channel *channel = find_channel_by_name(mask);

	if (channel != NULL)
	{
		const std::vector<int> &members = channel->get_members();
		std::vector<int>::size_type i = 0;

		while (i < members.size())
		{
			Client *member = find_client_by_fd(members[i]);

			if (member != NULL)
			{
				reply << ":" << server_name() << " 352 " << client->get_nickname() << " " << channel->get_name() << " " << member->get_username() << " " << member->get_ip() << " " << server_name() << " " << member->get_nickname() << " H";

				if (channel->is_operator(members[i]))
					reply << "@";

				reply << " :0 " << member->get_realname() << "\r\n";
			}
			i++;
		}
	}
	else
	{
		Client *target = find_client_by_nickname(mask);

		if (target != NULL)
			reply << ":" << server_name() << " 352 " << client->get_nickname() << " * " << target->get_username() << " " << target->get_ip() << " " << server_name() << " " << target->get_nickname() << " H :0 " << target->get_realname() << "\r\n";
	}

	reply << ":" << server_name() << " 315 " << client->get_nickname() << " " << mask << " :End of WHO list\r\n";
	queue_message(fd, reply.str());
}
