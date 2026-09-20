#include "Client.hpp"

Client::Client(int fd, const std::string &ip)
{
	_fd = fd;
	_ip = ip;
	_nickname.clear();
	_username.clear();
	_realname.clear();
	_passwordAccepted = false;
	_registered = false;
	_capNegotiating = false;
	_recvBuffer.clear();
	_sendBuffer.clear();
}

int	Client::get_fd() const
{
	return (_fd);
}

const std::string	&Client::get_ip() const
{
	return (_ip);
}

const std::string	&Client::get_nickname() const
{
	return (_nickname);
}

const std::string	&Client::get_username() const
{
	return (_username);
}

const std::string	&Client::get_realname() const
{
	return (_realname);
}

const std::string	&Client::get_recv_buffer() const
{
	return (_recvBuffer);
}

const std::string	&Client::get_send_buffer() const
{
	return (_sendBuffer);
}

bool	Client::is_password_accepted() const
{
	return (_passwordAccepted);
}

bool	Client::is_registered() const
{
	return (_registered);
}

bool	Client::is_cap_negotiating() const
{
	return (_capNegotiating);
}

bool	Client::has_pending_output() const
{
	return (!_sendBuffer.empty());
}

void	Client::set_nickname(const std::string &nickname)
{
	_nickname = nickname;
}

void	Client::set_username(const std::string &username)
{
	_username = username;
}

void	Client::set_realname(const std::string &realname)
{
	_realname = realname;
}

void	Client::set_password_accepted(bool accepted)
{
	_passwordAccepted = accepted;
}

void	Client::set_registered(bool registered)
{
	_registered = registered;
}

void	Client::set_cap_negotiating(bool negotiating)
{
	_capNegotiating = negotiating;
}

void	Client::append_received_data(const char *data, std::string::size_type size)
{
	_recvBuffer.append(data, size);
}

bool	Client::pop_next_command(std::string &command)
{
	std::string::size_type end = _recvBuffer.find('\n');

	if (end == std::string::npos)
		return (false);

	command = _recvBuffer.substr(0, end);
	_recvBuffer.erase(0, end + 1);

	if (!command.empty() && command[command.size() - 1] == '\r')
		command.erase(command.size() - 1);
	return (true);
}

void	Client::append_send_data(const std::string &data)
{
	_sendBuffer.append(data);
}

void	Client::consume_sent_data(std::string::size_type size)
{
	if (size >= _sendBuffer.size())
	{
		_sendBuffer.clear();
		return;
	}

	_sendBuffer.erase(0, size);
}