#include "Server.hpp"

#include <sys/epoll.h>
#include <unistd.h>

static bool	contains_fd(const std::vector<int> &fds, int fd)
{
	std::vector<int>::size_type i = 0;

	while (i < fds.size())
	{
		if (fds[i] == fd)
			return (true);
		i++;
	}
	return (false);
}

void	Server::handle_quit(int fd, const ParsedCommand &command)
{
	if (find_client_by_fd(fd) == NULL)
		return;

	std::string reason = "Client Quit";

	if (command.hasTrailing)
		reason = command.trailing;
	else if (!command.params.empty())
		reason = command.params[0];

	disconnect_client(fd, reason);
}

void	Server::disconnect_client(int fd, const std::string &reason)
{
	Client *client = find_client_by_fd(fd);

	if (client == NULL)
		return;

	std::vector<int> recipients;

	if (client->is_registered())
	{
		std::vector<Channel>::size_type channelIndex = 0;

		while (channelIndex < _channels.size())
		{
			if (_channels[channelIndex].has_member(fd))
			{
				const std::vector<int> &members = _channels[channelIndex].get_members();
				std::vector<int>::size_type memberIndex = 0;

				while (memberIndex < members.size())
				{
					if (members[memberIndex] != fd && !contains_fd(recipients, members[memberIndex]) && find_client_by_fd(members[memberIndex]) != NULL)
						recipients.push_back(members[memberIndex]);
					memberIndex++;
				}
			}
			channelIndex++;
		}

		std::string message = client_prefix(*client) + " QUIT :" + reason + "\r\n";
		std::vector<int>::size_type recipientIndex = 0;

		while (recipientIndex < recipients.size())
		{
			queue_message(recipients[recipientIndex], message);
			recipientIndex++;
		}
	}

	remove_client(fd);
}

void	Server::remove_client(int fd)
{
	if (_epollFD >= 0)
		epoll_ctl(_epollFD, EPOLL_CTL_DEL, fd, NULL);

	std::vector<Channel>::iterator channelIt = _channels.begin();

	while (channelIt != _channels.end())
	{
		channelIt->remove_invited(fd);
		channelIt->remove_member(fd);

		if (channelIt->is_empty())
			channelIt = _channels.erase(channelIt);
		else
			++channelIt;
	}

	std::vector<Client>::iterator clientIt = _clients.begin();

	while (clientIt != _clients.end())
	{
		if (clientIt->get_fd() == fd)
		{
			close(fd);
			_clients.erase(clientIt);
			resume_accepting();
			return;
		}
		++clientIt;
	}
}

bool	Server::is_disconnect_pending(int fd) const
{
	return (contains_fd(_pendingDisconnects, fd));
}

void	Server::mark_pending_disconnect(int fd)
{
	if (!is_disconnect_pending(fd))
		_pendingDisconnects.push_back(fd);
}

void	Server::process_pending_disconnects()
{
	std::vector<int>::size_type i = 0;

	while (i < _pendingDisconnects.size())
	{
		int fd = _pendingDisconnects[i];

		i++;
		disconnect_client(fd, "Connection closed");
	}

	_pendingDisconnects.clear();
}