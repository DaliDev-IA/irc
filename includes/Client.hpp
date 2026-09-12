#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client
{
	private:
		int				_fd;
		std::string		_ip;
		std::string		_nickname;
		std::string		_username;
		std::string		_realname;
		bool			_passwordAccepted;
		bool			_registered;
		bool			_capNegotiating;
		std::string		_recvBuffer;
		std::string		_sendBuffer;

	public:
		Client(int fd, const std::string &ip);

		int					get_fd() const;
		const std::string	&get_ip() const;
		const std::string	&get_nickname() const;
		const std::string	&get_username() const;
		const std::string	&get_realname() const;
		const std::string	&get_recv_buffer() const;
		const std::string	&get_send_buffer() const;

		bool	is_password_accepted() const;
		bool	is_registered() const;
		bool	is_cap_negotiating() const;
		bool	has_pending_output() const;

		void	set_nickname(const std::string &nickname);
		void	set_username(const std::string &username);
		void	set_realname(const std::string &realname);
		void	set_password_accepted(bool accepted);
		void	set_registered(bool registered);
		void	set_cap_negotiating(bool negotiating);

		void	append_received_data(const char *data, std::string::size_type size);
		bool	pop_next_command(std::string &command);

		void	append_send_data(const std::string &data);
		void	consume_sent_data(std::string::size_type size);
};

#endif