#include "Server.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <poll.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

Server::Server(int port, const std::string &password)
	: _port(port), _password(password), _listenFd(-1)
{
}

Server::~Server()
{
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		close(it->first);
	if (_listenFd != -1)
		close(_listenFd);
}

void	Server::setup()
{
	struct sockaddr_in	addr;
	int					on = 1;

	_listenFd = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenFd == -1)
		throw std::runtime_error("socket failed");
	setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	fcntl(_listenFd, F_SETFL, O_NONBLOCK);

	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(_port);
	if (bind(_listenFd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
		throw std::runtime_error("bind failed (port already in use?)");
	if (listen(_listenFd, 128) == -1)
		throw std::runtime_error("listen failed");
}

// one loop, one poll(): we only touch a socket when poll() says it is ready
void	Server::run()
{
	setup();
	std::cout << "listening on port " << _port << std::endl;
	while (g_running)
	{
		std::vector<struct pollfd>	fds;
		struct pollfd				pfd;

		pfd.fd = _listenFd;
		pfd.events = POLLIN;
		pfd.revents = 0;
		fds.push_back(pfd);
		for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		{
			pfd.fd = it->first;
			pfd.events = POLLIN;
			if (!it->second.outbuf.empty())
				pfd.events |= POLLOUT;
			fds.push_back(pfd);
		}

		if (poll(&fds[0], fds.size(), -1) == -1)
			continue ;

		if (fds[0].revents & POLLIN)
			acceptClient();
		for (size_t i = 1; i < fds.size(); i++)
		{
			Client	&client = _clients.find(fds[i].fd)->second;

			if (fds[i].revents & POLLIN)
				readClient(client);
			else if (fds[i].revents & (POLLHUP | POLLERR))
				disconnect(client, "Connection lost");
			if (fds[i].revents & POLLOUT)
				writeClient(client);
		}
		removeDeadClients();
	}
	std::cout << "server stopped" << std::endl;
}

void	Server::acceptClient()
{
	struct sockaddr_in	addr;
	socklen_t			len = sizeof(addr);
	int					fd;

	fd = accept(_listenFd, (struct sockaddr *)&addr, &len);
	if (fd == -1)
		return ;
	fcntl(fd, F_SETFL, O_NONBLOCK);
	_clients.insert(std::make_pair(fd, Client(fd, inet_ntoa(addr.sin_addr))));
	std::cout << "client " << fd << " connected" << std::endl;
}

// data can arrive in pieces, so we keep it in inbuf until we have a full line
void	Server::readClient(Client &client)
{
	char	buf[1024];
	ssize_t	n;
	size_t	pos;

	n = recv(client.fd, buf, sizeof(buf), 0);
	if (n == 0)
		disconnect(client, "Connection closed");
	if (n <= 0)
		return ;
	client.inbuf.append(buf, n);
	while (!client.dead && (pos = client.inbuf.find('\n')) != std::string::npos)
	{
		std::string	line = client.inbuf.substr(0, pos);

		client.inbuf.erase(0, pos + 1);
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		handleLine(client, line);
	}
}

// send() may not take everything: what is left stays in outbuf for next time
void	Server::writeClient(Client &client)
{
	ssize_t	n;

	n = send(client.fd, client.outbuf.c_str(), client.outbuf.size(), 0);
	if (n > 0)
		client.outbuf.erase(0, n);
}

void	Server::removeDeadClients()
{
	std::map<int, Client>::iterator	it = _clients.begin();

	while (it != _clients.end())
	{
		if (it->second.dead)
		{
			std::cout << "client " << it->first << " disconnected" << std::endl;
			close(it->first);
			_clients.erase(it++);
		}
		else
			++it;
	}
}
