#include "Channel.hpp"

Channel::Channel() : limit(0), inviteOnly(false), topicLocked(false)
{
}

bool	Channel::has(int fd) const
{
	return members.count(fd) > 0;
}

bool	Channel::isOperator(int fd) const
{
	return operators.count(fd) > 0;
}

void	Channel::remove(int fd)
{
	members.erase(fd);
	operators.erase(fd);
	invited.erase(fd);
}

std::string	Channel::modes() const
{
	std::string result = "+";

	if (inviteOnly)
		result += "i";
	if (topicLocked)
		result += "t";
	if (!key.empty())
		result += "k";
	if (limit > 0)
		result += "l";
	return result;
}
