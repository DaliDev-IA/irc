#include "Server.hpp"

#include <iostream>
#include <sstream>
#include <string>

int	main(int argc, char **argv)
{
	if (argc != 3)
	{
		std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
		return (1);
	}

	long portValue;
	std::string extra;
	std::stringstream portStream(argv[1]);

	if (!(portStream >> portValue) || (portStream >> extra) || portValue < 1 || portValue > 65535)
	{
		std::cerr << "Error: invalid port" << std::endl;
		return (1);
	}

	try
	{
		Server server(static_cast<unsigned int>(portValue), std::string(argv[2]));
		server.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return (1);
	}

	return (0);
}