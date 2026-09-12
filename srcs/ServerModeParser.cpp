#include "Server.hpp"

#include <sstream>

void	Server::apply_channel_modes(int fd, Channel &channel, const ParsedCommand &command)
{
	Client *client = find_client_by_fd(fd);

	if (client == NULL)
		return;

	const std::string &modeString = command.params[1];

	bool adding = true;
	bool hasSign = false;
	char lastSign = '\0';
	std::string appliedModes;
	std::string appliedParams;
	std::vector<std::string>::size_type paramIndex = 2;
	std::string::size_type i = 0;

	while (i < modeString.size())
	{
		char mode = modeString[i];

		if (mode == '+' || mode == '-')
		{
			adding = (mode == '+');
			hasSign = true;
			i++;
			continue;
		}

		if (!hasSign)
		{
			std::ostringstream reply;
			reply << ":" << server_name() << " 472 " << client->get_nickname() << " " << mode << " :is unknown mode char to me\r\n";
			queue_message(fd, reply.str());
			i++;
			continue;
		}

		bool changed = false;
		std::string appliedParam;

		if (mode == 'i' || mode == 't')
			changed = apply_simple_mode(channel, mode, adding);
		else if (mode == 'k' || mode == 'l')
			changed = apply_param_mode(fd, channel, mode, adding, command, paramIndex, appliedParam);
		else if (mode == 'o')
			changed = apply_operator_mode(fd, channel, adding, command, paramIndex, appliedParam);
		else
		{
			std::ostringstream reply;
			reply << ":" << server_name() << " 472 " << client->get_nickname() << " " << mode << " :is unknown mode char to me\r\n";
			queue_message(fd, reply.str());
		}

		if (changed)
		{
			char currentSign = adding ? '+' : '-';

			if (lastSign != currentSign)
			{
				appliedModes += currentSign;
				lastSign = currentSign;
			}

			appliedModes += mode;

			if (!appliedParam.empty())
				appliedParams += " " + appliedParam;
		}

		i++;
	}

	if (appliedModes.empty())
		return;

	std::string message = client_prefix(*client) + " MODE " + channel.get_name() + " " + appliedModes + appliedParams + "\r\n";
	broadcast_to_channel(channel, message, -1);
}