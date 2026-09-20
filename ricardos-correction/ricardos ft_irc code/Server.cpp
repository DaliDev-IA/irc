#include "Server.hpp"
#include <arpa/inet.h>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <poll.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

// Becomes 0 when we receive ctrl+C, and the main loop ends.
static volatile sig_atomic_t	g_running = 1;

void	Server::stop(int signal)
{
	(void)signal;
	g_running = 0;
}

Server::Server(int port, const std::string &password)
	: _port(port), _password(password), _listenFd(-1)
{
}

Server::~Server()
{
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		close(it->first);
	if (_listenFd >= 0)
		close(_listenFd);
}

// Create the socket that waits for new connections.
void	Server::openSocket()
{
	_listenFd = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenFd < 0)
		throw std::runtime_error("socket() failed");

	int yes = 1;	// lets us restart the server right away on the same port
	if (setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
		throw std::runtime_error("setsockopt() failed");
	if (fcntl(_listenFd, F_SETFL, O_NONBLOCK) < 0)
		throw std::runtime_error("fcntl() failed");

	struct sockaddr_in address;
	std::memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(_port);

	if (bind(_listenFd, reinterpret_cast<struct sockaddr *>(&address), sizeof(address)) < 0)
		throw std::runtime_error("bind() failed, is the port already used?");
	if (listen(_listenFd, SOMAXCONN) < 0)
		throw std::runtime_error("listen() failed");
}

// The heart of the server. One loop, one poll().
//   1. make the list of sockets we want to watch
//   2. poll() sleeps until at least one of them is ready
//   3. we read / write / accept only on the sockets poll() marked as ready
void	Server::run()
{
	openSocket();
	std::cout << "Listening on port " << _port << " (ctrl+C to stop)" << std::endl;

	while (g_running)
	{
		std::vector<struct pollfd> watched;
		struct pollfd entry;

		entry.fd = _listenFd;
		entry.events = (_clients.size() < MAX_CLIENTS) ? POLLIN : 0;
		entry.revents = 0;
		watched.push_back(entry);

		for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		{
			entry.fd = it->first;
			entry.events = POLLIN;
			if (!it->second.outbox.empty())		// we only ask "can I write?" when
				entry.events |= POLLOUT;		// we really have something to send
			watched.push_back(entry);
		}

		if (poll(&watched[0], watched.size(), -1) < 0)
			continue;	// interrupted by a signal: the while() checks g_running

		if (watched[0].revents & POLLIN)
			acceptClient();

		for (size_t i = 1; i < watched.size(); i++)
		{
			Client &client = _clients.find(watched[i].fd)->second;

			if (watched[i].revents & POLLIN)
				readFrom(client);
			else if (watched[i].revents & (POLLERR | POLLHUP | POLLNVAL))
				disconnect(client, "Connection lost");
			if (watched[i].revents & POLLOUT)
				writeTo(client);
		}
		removeDeadClients();
	}
	std::cout << std::endl << "Server stopped." << std::endl;
}

void	Server::acceptClient()
{
	struct sockaddr_in	address;
	socklen_t			size = sizeof(address);

	int fd = accept(_listenFd, reinterpret_cast<struct sockaddr *>(&address), &size);
	if (fd < 0)
		return;
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
	{
		close(fd);
		return;
	}
	_clients.insert(std::make_pair(fd, Client(fd, inet_ntoa(address.sin_addr))));
	std::cout << "New client on fd " << fd << std::endl;
}

// Data can arrive in pieces ("JO", then "IN #a\r\n"), so we add every piece
// to the client's inbox and only handle the lines that are complete.
void	Server::readFrom(Client &client)
{
	char	buffer[512];
	ssize_t	size = recv(client.fd, buffer, sizeof(buffer), 0);

	if (size == 0)
		disconnect(client, "Connection closed");
	if (size <= 0)
		return;
	client.inbox.append(buffer, size);

	size_t end;
	while (!client.dead && (end = client.inbox.find('\n')) != std::string::npos)
	{
		std::string line = client.inbox.substr(0, end);
		client.inbox.erase(0, end + 1);
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		handleLine(client, line);
	}
	if (client.inbox.size() > MAX_LINE)
		disconnect(client, "Line too long");
}

// send() may take only a part of the outbox: we keep the rest for next time.
void	Server::writeTo(Client &client)
{
	if (client.dead)
		return;
	ssize_t size = send(client.fd, client.outbox.c_str(), client.outbox.size(), 0);
	if (size > 0)
		client.outbox.erase(0, size);
}

// Clients are never deleted in the middle of the loop (it would break the
// references we are using). They are marked "dead" and deleted here.
void	Server::removeDeadClients()
{
	std::map<int, Client>::iterator it = _clients.begin();

	while (it != _clients.end())
	{
		if (it->second.dead)
		{
			std::cout << "Client on fd " << it->first << " left" << std::endl;
			close(it->first);
			_clients.erase(it++);
		}
		else
			++it;
	}
}
