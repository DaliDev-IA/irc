#include "Server.hpp"

bool	Server::apply_simple_mode(Channel &channel, char mode, bool adding)
{
	if (mode == 'i')
	{
		if (channel.is_invite_only() == adding)
			return (false);

		channel.set_invite_only(adding);
		return (true);
	}

	if (mode == 't')
	{
		if (channel.is_topic_restricted() == adding)
			return (false);

		channel.set_topic_restricted(adding);
		return (true);
	}

	return (false);
}