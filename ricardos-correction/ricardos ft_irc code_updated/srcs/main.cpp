#include "Server.hpp"
#include <cstdlib>
#include <iostream>

volatile sig_atomic_t	g_running = 1;

static void	stopServer(int)
{
	g_running = 0;
}

static bool	validPort(const std::string &s)
{
	if (!isNumber(s) || s.size() > 5)
		return (false);
	int	port = std::atoi(s.c_str());
	return (port > 0 && port <= 65535);
}

int	main(int argc, char **argv)
{
	if (argc != 3 || !validPort(argv[1]) || !argv[2][0])
	{
		std::cerr << "usage: ./ircserv <port> <password>" << std::endl;
		return (1);
	}
	signal(SIGINT, stopServer);
	signal(SIGTERM, stopServer);
	signal(SIGPIPE, SIG_IGN);
	try
	{
		Server	server(std::atoi(argv[1]), argv[2]);
		server.run();
	}
	catch (std::exception &e)
	{
		std::cerr << "error: " << e.what() << std::endl;
		return (1);
	}
	return (0);
}
