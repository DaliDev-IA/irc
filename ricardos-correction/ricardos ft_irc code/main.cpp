#include "Server.hpp"
#include <csignal>
#include <cstdlib>
#include <iostream>

static bool	isValidPort(const std::string &text)
{
	if (text.empty() || text.size() > 5)
		return false;
	for (size_t i = 0; i < text.size(); i++)
		if (text[i] < '0' || text[i] > '9')
			return false;
	int port = std::atoi(text.c_str());
	return port >= 1 && port <= 65535;
}

int	main(int argc, char **argv)
{
	if (argc != 3 || !isValidPort(argv[1]) || std::string(argv[2]).empty())
	{
		std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
		return 1;
	}

	std::signal(SIGINT, Server::stop);		// ctrl+C  -> stop the server cleanly
	std::signal(SIGTERM, Server::stop);
	std::signal(SIGQUIT, Server::stop);
	std::signal(SIGPIPE, SIG_IGN);			// a client that vanished must not kill us

	try
	{
		Server server(std::atoi(argv[1]), argv[2]);
		server.run();
	}
	catch (const std::exception &error)
	{
		std::cerr << "Error: " << error.what() << std::endl;
		return 1;
	}
	return 0;
}
