#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <string>

// everything the server knows about one connected user
class Client
{
	public:
		int			fd;
		std::string	host;
		std::string	nick;
		std::string	user;
		bool		passOk;		// gave the right password
		bool		registered;	// PASS + NICK + USER done
		bool		dead;		// to be closed at the end of the loop
		std::string	inbuf;		// bytes received, waiting for a full line
		std::string	outbuf;		// bytes waiting to be sent

		Client(int fd, const std::string &host);

		std::string	prefix() const;	// nick!user@host
};

#endif
