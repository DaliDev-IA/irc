#include "Channel.hpp"

Channel::Channel() : limit(0), inviteOnly(false), topicOps(false)
{
}

bool	Channel::has(int fd) const
{
	return (members.count(fd) > 0);
}

bool	Channel::isOp(int fd) const
{
	return (ops.count(fd) > 0);
}

std::string	Channel::modes() const
{
	std::string	s = "+";

	if (inviteOnly)
		s += "i";
	if (topicOps)
		s += "t";
	if (!key.empty())
		s += "k";
	if (limit > 0)
		s += "l";
	return (s);
}
