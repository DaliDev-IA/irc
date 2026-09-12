#include "Server.hpp"

const char	*Server::server_name() const
{
	return ("ircserv");
}

char	Server::ascii_to_lower(char c) const
{
	if (c >= 'A' && c <= 'Z')
		return (c - 'A' + 'a');
	return (c);
}

bool	Server::ascii_case_equal(const std::string &left, const std::string &right) const
{
	if (left.size() != right.size())
		return (false);

	std::string::size_type i = 0;

	while (i < left.size())
	{
		if (ascii_to_lower(left[i]) != ascii_to_lower(right[i]))
			return (false);
		i++;
	}
	return (true);
}

bool	Server::is_nick_special(char c) const
{
	return (c == '[' || c == ']' || c == '\\' || c == '`' || c == '_' || c == '^' || c == '{' || c == '|' || c == '}');
}

std::string	Server::client_target(const Client &client) const
{
	if (client.get_nickname().empty())
		return ("*");
	return (client.get_nickname());
}

std::string	Server::client_prefix(const Client &client) const
{
	return (":" + client.get_nickname() + "!" + client.get_username() + "@" + client.get_ip());
}