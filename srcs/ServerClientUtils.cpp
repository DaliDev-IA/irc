#include "Server.hpp"

Client	*Server::find_client_by_nickname(const std::string &nickname)
{
	std::vector<Client>::size_type i = 0;

	while (i < _clients.size())
	{
		if (_clients[i].is_registered() && ascii_case_equal(_clients[i].get_nickname(), nickname))
			return (&_clients[i]);
		i++;
	}
	return (NULL);
}

bool	Server::is_nickname_valid(const std::string &nickname) const
{
	if (nickname.empty())
		return (false);

	char first = nickname[0];
	bool firstIsLetter = (first >= 'A' && first <= 'Z') || (first >= 'a' && first <= 'z');

	if (!firstIsLetter && !is_nick_special(first))
		return (false);

	std::string::size_type i = 1;

	while (i < nickname.size())
	{
		char c = nickname[i];
		bool isLetter = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
		bool isDigit = c >= '0' && c <= '9';

		if (!isLetter && !isDigit && !is_nick_special(c) && c != '-')
			return (false);
		i++;
	}
	return (true);
}

bool	Server::is_nickname_available(const std::string &nickname, int currentFD) const
{
	std::vector<Client>::size_type i = 0;

	while (i < _clients.size())
	{
		if (_clients[i].get_fd() != currentFD && !_clients[i].get_nickname().empty()
			&& ascii_case_equal(_clients[i].get_nickname(), nickname))
			return (false);
		i++;
	}
	return (true);
}