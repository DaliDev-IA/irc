#include "Server.hpp"

#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>

void	Server::set_socket_non_blocking(int fd)
{
	int flags = fcntl(fd, F_GETFL);

	if (flags == -1)
		throw std::runtime_error("fcntl F_GETFL error");

	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		throw std::runtime_error("fcntl F_SETFL error");
}

bool	Server::add_fd_to_epoll(int fd)
{
	struct epoll_event event;

	std::memset(&event, 0, sizeof(event));
	event.events = EPOLLIN;

	if (fd != _socketFD)
		event.events |= EPOLLRDHUP;

	event.data.fd = fd;

	return (epoll_ctl(_epollFD, EPOLL_CTL_ADD, fd, &event) != -1);
}

bool	Server::update_client_epoll(int fd, bool wantWrite)
{
	struct epoll_event event;

	std::memset(&event, 0, sizeof(event));
	event.events = EPOLLIN | EPOLLRDHUP;

	if (wantWrite)
		event.events |= EPOLLOUT;

	event.data.fd = fd;

	return (epoll_ctl(_epollFD, EPOLL_CTL_MOD, fd, &event) != -1);
}

void	Server::epoll_init()
{
	_epollFD = epoll_create1(0);

	if (_epollFD == -1)
		throw std::runtime_error("epoll_create1 error");

	if (!add_fd_to_epoll(_socketFD))
		throw std::runtime_error("epoll_ctl add error");
}

void	Server::pause_accepting()
{
	if (_acceptPaused)
		return;

	epoll_ctl(_epollFD, EPOLL_CTL_DEL, _socketFD, NULL);
	_acceptPaused = true;
}

void	Server::resume_accepting()
{
	if (!_acceptPaused)
		return;

	if (add_fd_to_epoll(_socketFD))
		_acceptPaused = false;
}

void	Server::accept_new_client()
{
	struct sockaddr_in clientAddress;
	socklen_t clientAddressSize = sizeof(clientAddress);

	std::memset(&clientAddress, 0, sizeof(clientAddress));

	int clientFD = accept(_socketFD, reinterpret_cast<struct sockaddr *>(&clientAddress), &clientAddressSize);

	if (clientFD == -1)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return;

		if (errno == EMFILE || errno == ENFILE)
			pause_accepting();

		return;
	}

	try
	{
		set_socket_non_blocking(clientFD);

		char clientIP[INET_ADDRSTRLEN];

		if (inet_ntop(AF_INET, &clientAddress.sin_addr, clientIP, sizeof(clientIP)) == NULL)
			throw std::runtime_error("inet_ntop error");

		if (!add_fd_to_epoll(clientFD))
			throw std::runtime_error("epoll_ctl add error");

		_clients.push_back(Client(clientFD, std::string(clientIP)));
	}
	catch (...)
	{
		close(clientFD);
	}
}

void	Server::receive_client_data(int fd)
{
	Client *client = find_client_by_fd(fd);

	if (client == NULL)
		return;

	char buffer[512];
	ssize_t bytesReceived = recv(fd, buffer, sizeof(buffer), 0);

	if (bytesReceived > 0)
	{
		client->append_received_data(buffer, static_cast<std::string::size_type>(bytesReceived));

		Parser parser;
		std::string line;

		while (1)
		{
			client = find_client_by_fd(fd);

			if (client == NULL)
				return;

			if (!client->pop_next_command(line))
				break;

			ParsedCommand parsed = parser.parse(line);
			process_command(fd, parsed);
		}
		return;
	}

	if (bytesReceived == 0)
	{
		disconnect_client(fd, "Connection closed");
		return;
	}

	return;
}

void	Server::queue_message(int fd, const std::string &message)
{
	Client *client = find_client_by_fd(fd);

	if (client == NULL)
		return;

	client->append_send_data(message);

	if (!update_client_epoll(fd, true))
		disconnect_client(fd, "Connection closed");
}

void	Server::send_client_data(int fd)
{
	Client *client = find_client_by_fd(fd);

	if (client == NULL)
		return;

	if (!client->has_pending_output())
	{
		if (!update_client_epoll(fd, false))
			disconnect_client(fd, "Connection closed");
		return;
	}

	const std::string &output = client->get_send_buffer();
	ssize_t bytesSent = send(fd, output.c_str(), output.size(), MSG_NOSIGNAL);

	if (bytesSent <= 0)
		return;

	client->consume_sent_data(static_cast<std::string::size_type>(bytesSent));

	if (!client->has_pending_output())
	{
		if (!update_client_epoll(fd, false))
			disconnect_client(fd, "Connection closed");
	}
}