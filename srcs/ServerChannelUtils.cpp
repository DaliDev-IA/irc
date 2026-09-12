#include "Server.hpp"

#include <sstream>

Channel	*Server::find_channel_by_name(const std::string &name)
{
	std::vector<Channel>::size_type i = 0;

	while (i < _channels.size())
	{
		if (ascii_case_equal(_channels[i].get_name(), name))
			return (&_channels[i]);
		i++;
	}
	return (NULL);
}

void	Server::remove_channel_if_empty(const std::string &name)
{
	std::vector<Channel>::iterator it = _channels.begin();

	while (it != _channels.end())
	{
		if (ascii_case_equal(it->get_name(), name) && it->is_empty())
		{
			_channels.erase(it);
			return;
		}
		++it;
	}
}

bool	Server::is_channel_name_valid(const std::string &name) const
{
	if (name.size() < 2 || name[0] != '#')
		return (false);

	std::string::size_type i = 1;

	while (i < name.size())
	{
		if (name[i] == ' ' || name[i] == ',' || name[i] == ':' || name[i] == '\a')
			return (false);
		i++;
	}
	return (true);
}

void	Server::broadcast_to_channel(const Channel &channel, const std::string &message, int exceptFD)
{
	const std::vector<int> &members = channel.get_members();
	std::vector<int>::size_type i = 0;

	while (i < members.size())
	{
		int memberFD = members[i];

		if (memberFD != exceptFD && find_client_by_fd(memberFD) != NULL)
			queue_message(memberFD, message);
		i++;
	}
}

void	Server::send_names_reply(int fd, const Channel &channel)
{
	Client *client = find_client_by_fd(fd);

	if (client == NULL)
		return;

	std::ostringstream reply;

	reply << ":" << server_name() << " 353 " << client->get_nickname() << " = " << channel.get_name() << " :";

	const std::vector<int> &members = channel.get_members();
	std::vector<int>::size_type i = 0;
	bool first = true;

	while (i < members.size())
	{
		Client *member = find_client_by_fd(members[i]);

		if (member != NULL)
		{
			if (!first)
				reply << " ";

			if (channel.is_operator(members[i]))
				reply << "@";

			reply << member->get_nickname();
			first = false;
		}
		i++;
	}

	reply << "\r\n";
	reply << ":" << server_name() << " 366 " << client->get_nickname() << " " << channel.get_name() << " :End of /NAMES list\r\n";

	queue_message(fd, reply.str());
}