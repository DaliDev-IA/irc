#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <string>

// One connected user. It is only data, so everything is public.
class Client
{
	public:
		int			fd;
		std::string	host;
		std::string	nick;
		std::string	user;
		std::string	realname;
		bool		passOk;			// sent the right password
		bool		registered;		// PASS + NICK + USER are all done
		bool		dead;			// will be removed at the end of the loop
		std::string	inbox;			// bytes received, not a full line yet
		std::string	outbox;			// bytes waiting to be sent

		Client(int fd, const std::string &host);

		std::string	prefix() const;	// "nick!user@host"
};

#endif
