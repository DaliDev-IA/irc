#include "Server.hpp"

#include <limits>
#include <sstream>

static bool	parse_limit(const std::string &text, unsigned int &limit)
{
	if (text.empty())
		return (false);

	unsigned long value = 0;
	unsigned long maxValue = std::numeric_limits<unsigned int>::max();
	std::string::size_type i = 0;

	while (i < text.size())
	{
		if (text[i] < '0' || text[i] > '9')
			return (false);

		unsigned int digit = text[i] - '0';

		if (value > (maxValue - digit) / 10)
			return (false);

		value = value * 10 + digit;
		i++;
	}

	if (value == 0)
		return (false);

	limit = static_cast<unsigned int>(value);
	return (true);
}

bool	Server::apply_param_mode(int fd, Channel &channel, char mode, bool adding, const ParsedCommand &command, std::vector<std::string>::size_type &paramIndex, std::string &appliedParam)
{
	Client *client = find_client_by_fd(fd);

	if (client == NULL)
		return (false);

	if (mode == 'k')
	{
		if (adding)
		{
			if (paramIndex >= command.params.size())
			{
				std::ostringstream reply;
				reply << ":" << server_name() << " 461 " << client->get_nickname() << " MODE :Not enough parameters\r\n";
				queue_message(fd, reply.str());
				return (false);
			}

			const std::string &key = command.params[paramIndex];
			paramIndex++;

			if (key.empty())
				return (false);

			if (channel.has_key() && channel.get_key() == key)
				return (false);

			channel.set_key(key);
			appliedParam = key;
			return (true);
		}

		if (!channel.has_key())
		{
			if (paramIndex < command.params.size())
				paramIndex++;
			return (false);
		}

		appliedParam = channel.get_key();
		channel.remove_key();

		if (paramIndex < command.params.size())
			paramIndex++;

		return (true);
	}

	if (mode == 'l')
	{
		if (!adding)
		{
			if (!channel.has_user_limit())
				return (false);

			channel.remove_user_limit();
			return (true);
		}

		if (paramIndex >= command.params.size())
		{
			std::ostringstream reply;
			reply << ":" << server_name() << " 461 " << client->get_nickname() << " MODE :Not enough parameters\r\n";
			queue_message(fd, reply.str());
			return (false);
		}

		const std::string &limitText = command.params[paramIndex];
		paramIndex++;

		unsigned int limit;

		if (!parse_limit(limitText, limit))
		{
			std::ostringstream reply;
			reply << ":" << server_name() << " 696 " << client->get_nickname() << " " << channel.get_name() << " l " << limitText << " :Invalid mode parameter\r\n";
			queue_message(fd, reply.str());
			return (false);
		}

		if (channel.has_user_limit() && channel.get_user_limit() == limit)
			return (false);

		channel.set_user_limit(limit);
		appliedParam = limitText;
		return (true);
	}

	return (false);
}