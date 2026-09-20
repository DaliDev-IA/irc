#include "Client.hpp"

Client::Client(int fd, const std::string &host)
	: fd(fd), host(host), passOk(false), registered(false), dead(false)
{
}

std::string	Client::prefix() const
{
	return nick + "!" + user + "@" + host;
}
