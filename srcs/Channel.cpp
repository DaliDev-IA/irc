#include "Channel.hpp"

Channel::Channel(const std::string &name)
	: _name(name), _topic(""), _key(""), _userLimit(0), _inviteOnly(false), _topicRestricted(false)
{
}

const std::string	&Channel::get_name() const
{
	return (_name);
}

const std::string	&Channel::get_topic() const
{
	return (_topic);
}

const std::vector<int>	&Channel::get_members() const
{
	return (_members);
}

bool	Channel::has_topic() const
{
	return (!_topic.empty());
}

void	Channel::set_topic(const std::string &topic)
{
	_topic = topic;
}

bool	Channel::is_empty() const
{
	return (_members.empty());
}

bool	Channel::has_member(int fd) const
{
	std::vector<int>::size_type i = 0;

	while (i < _members.size())
	{
		if (_members[i] == fd)
			return (true);
		i++;
	}
	return (false);
}

void	Channel::add_member(int fd)
{
	if (!has_member(fd))
		_members.push_back(fd);
}

void	Channel::remove_member(int fd)
{
	remove_operator(fd);

	std::vector<int>::iterator it = _members.begin();

	while (it != _members.end())
	{
		if (*it == fd)
		{
			_members.erase(it);
			return;
		}
		++it;
	}
}

bool	Channel::is_operator(int fd) const
{
	std::vector<int>::size_type i = 0;

	while (i < _operators.size())
	{
		if (_operators[i] == fd)
			return (true);
		i++;
	}
	return (false);
}

void	Channel::add_operator(int fd)
{
	if (has_member(fd) && !is_operator(fd))
		_operators.push_back(fd);
}

void	Channel::remove_operator(int fd)
{
	std::vector<int>::iterator it = _operators.begin();

	while (it != _operators.end())
	{
		if (*it == fd)
		{
			_operators.erase(it);
			return;
		}
		++it;
	}
}

bool	Channel::is_invited(int fd) const
{
	std::vector<int>::size_type i = 0;

	while (i < _invited.size())
	{
		if (_invited[i] == fd)
			return (true);
		i++;
	}
	return (false);
}

void	Channel::add_invited(int fd)
{
	if (!is_invited(fd))
		_invited.push_back(fd);
}

void	Channel::remove_invited(int fd)
{
	std::vector<int>::iterator it = _invited.begin();

	while (it != _invited.end())
	{
		if (*it == fd)
		{
			_invited.erase(it);
			return;
		}
		++it;
	}
}

bool	Channel::is_invite_only() const
{
	return (_inviteOnly);
}

void	Channel::set_invite_only(bool value)
{
	_inviteOnly = value;
}

bool	Channel::is_topic_restricted() const
{
	return (_topicRestricted);
}

void	Channel::set_topic_restricted(bool value)
{
	_topicRestricted = value;
}

bool	Channel::has_key() const
{
	return (!_key.empty());
}

const std::string	&Channel::get_key() const
{
	return (_key);
}

void	Channel::set_key(const std::string &key)
{
	_key = key;
}

void	Channel::remove_key()
{
	_key.clear();
}

bool	Channel::has_user_limit() const
{
	return (_userLimit > 0);
}

unsigned int	Channel::get_user_limit() const
{
	return (_userLimit);
}

void	Channel::set_user_limit(unsigned int limit)
{
	_userLimit = limit;
}

void	Channel::remove_user_limit()
{
	_userLimit = 0;
}