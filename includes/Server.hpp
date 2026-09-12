#ifndef SERVER_HPP
#define SERVER_HPP

#include "Client.hpp"
#include "Parser.hpp"
#include "Channel.hpp"

#include <netinet/in.h>
#include <string>
#include <vector>

class Server
{
	private:
		enum
		{
			MAX_EVENTS = 10
		};

		unsigned int			_port;
		std::string				_password;
		int						_socketFD;
		int						_epollFD;
		bool					_acceptPaused;
		struct sockaddr_in		_address;

		std::vector<Client>		_clients;
		std::vector<Channel>	_channels;
		std::vector<int>		_pendingDisconnects;

		Server(const Server &other);
		Server	&operator=(const Server &other);

		void	setup_server();
		void	listening_loop();

		void	set_socket_non_blocking(int fd);
		void	epoll_init();
		bool	add_fd_to_epoll(int fd);
		bool	update_client_epoll(int fd, bool wantWrite);
		void	pause_accepting();
		void	resume_accepting();
		void	accept_new_client();
		void	receive_client_data(int fd);
		void	send_client_data(int fd);
		void	queue_message(int fd, const std::string &message);

		Client	*find_client_by_fd(int fd);
		Client	*find_client_by_nickname(const std::string &nickname);

		void	disconnect_client(int fd, const std::string &reason);
		void	remove_client(int fd);

		bool	is_disconnect_pending(int fd) const;
		void	mark_pending_disconnect(int fd);
		void	process_pending_disconnects();

		Channel	*find_channel_by_name(const std::string &name);
		void	remove_channel_if_empty(const std::string &name);

		void	process_command(int fd, const ParsedCommand &command);

		void	handle_pass(int fd, const ParsedCommand &command);
		void	handle_nick(int fd, const ParsedCommand &command);
		void	handle_user(int fd, const ParsedCommand &command);
		void	handle_cap(int fd, const ParsedCommand &command);
		void	handle_ping(int fd, const ParsedCommand &command);

		void	handle_join(int fd, const ParsedCommand &command);
		void	handle_part(int fd, const ParsedCommand &command);
		void	handle_topic(int fd, const ParsedCommand &command);
		void	handle_kick(int fd, const ParsedCommand &command);
		void	handle_invite(int fd, const ParsedCommand &command);
		void	handle_privmsg(int fd, const ParsedCommand &command);
		void	handle_quit(int fd, const ParsedCommand &command);

		void	handle_mode(int fd, const ParsedCommand &command);
		void	send_channel_modes(int fd, const Channel &channel);
		void	apply_channel_modes(int fd, Channel &channel, const ParsedCommand &command);
		bool	apply_simple_mode(Channel &channel, char mode, bool adding);
		bool	apply_param_mode(int fd, Channel &channel, char mode, bool adding, const ParsedCommand &command, std::vector<std::string>::size_type &paramIndex, std::string &appliedParam);
		bool	apply_operator_mode(int fd, Channel &channel, bool adding, const ParsedCommand &command, std::vector<std::string>::size_type &paramIndex, std::string &appliedParam);

		void	try_register_client(int fd);

		const char	*server_name() const;
		char		ascii_to_lower(char c) const;
		bool		ascii_case_equal(const std::string &left, const std::string &right) const;
		bool		is_nick_special(char c) const;
		std::string	client_target(const Client &client) const;
		std::string	client_prefix(const Client &client) const;

		bool	is_nickname_valid(const std::string &nickname) const;
		bool	is_nickname_available(const std::string &nickname, int currentFD) const;
		bool	is_channel_name_valid(const std::string &name) const;

		void	broadcast_to_channel(const Channel &channel, const std::string &message, int exceptFD);
		void	send_names_reply(int fd, const Channel &channel);

	public:
		Server(unsigned int port, const std::string &password);
		~Server();

		void	run();
};

#endif