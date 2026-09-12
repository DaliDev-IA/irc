#include "Server.hpp"

#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>

Server::Server(unsigned int port, const std::string &password)
{
	_port = port;
	_password = password;
	_socketFD = -1;
	_epollFD = -1;
	_acceptPaused = false;
	std::memset(&_address, 0, sizeof(_address));
}

Server::~Server()
{
	std::vector<Client>::size_type i = 0;

	while (i < _clients.size())
	{
		if (_clients[i].get_fd() >= 0)
			close(_clients[i].get_fd());
		i++;
	}

	if (_socketFD >= 0)
		close(_socketFD);

	if (_epollFD >= 0)
		close(_epollFD);
}

void	Server::run()
{
	setup_server();
	listening_loop();
}

void	Server::setup_server()
{
	_socketFD = socket(AF_INET, SOCK_STREAM, 0);

	if (_socketFD == -1)
		throw std::runtime_error("socket creation error");

	_address.sin_family = AF_INET;
	_address.sin_addr.s_addr = INADDR_ANY;
	_address.sin_port = htons(_port);

	int enable = 1;

	if (setsockopt(_socketFD, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1)
		throw std::runtime_error("setsockopt SO_REUSEADDR error");

	set_socket_non_blocking(_socketFD);
}

void	Server::listening_loop()
{
	if (bind(_socketFD, reinterpret_cast<struct sockaddr *>(&_address), sizeof(_address)) == -1)
		throw std::runtime_error("bind error");

	if (listen(_socketFD, SOMAXCONN) == -1)
		throw std::runtime_error("listen error");

	epoll_init();

	struct epoll_event events[MAX_EVENTS];

	while (1)
	{
		int eventCount = epoll_wait(_epollFD, events, MAX_EVENTS, -1);

		if (eventCount == -1)
		{
			if (errno == EINTR)
				continue;

			throw std::runtime_error("epoll_wait error");
		}

		int i = 0;

		while (i < eventCount)
		{
			int currentFD = events[i].data.fd;

			if (currentFD == _socketFD)
			{
				if (events[i].events & EPOLLIN)
					accept_new_client();
			}
			else
			{
				bool hadInput = (events[i].events & EPOLLIN);

				if (hadInput)
					receive_client_data(currentFD);

				if (find_client_by_fd(currentFD) != NULL && (events[i].events & EPOLLOUT))
					send_client_data(currentFD);

				if (find_client_by_fd(currentFD) != NULL && (events[i].events & (EPOLLERR | EPOLLHUP)))
					disconnect_client(currentFD, "Connection closed");
				else if (find_client_by_fd(currentFD) != NULL && (events[i].events & EPOLLRDHUP) && !hadInput)
					disconnect_client(currentFD, "Connection closed");
			}

			i++;
		}
	}
}

Client	*Server::find_client_by_fd(int fd)
{
	std::vector<Client>::size_type i = 0;

	while (i < _clients.size())
	{
		if (_clients[i].get_fd() == fd)
			return (&_clients[i]);

		i++;
	}

	return (NULL);
}