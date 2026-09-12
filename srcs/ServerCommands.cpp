#include "Server.hpp"

void	Server::process_command(int fd, const ParsedCommand &command)
{
	if (find_client_by_fd(fd) == NULL || command.command.empty())
		return;

	if (ascii_case_equal(command.command, "CAP"))
		handle_cap(fd, command);
	else if (ascii_case_equal(command.command, "PING"))
		handle_ping(fd, command);
	else if (ascii_case_equal(command.command, "PASS"))
		handle_pass(fd, command);
	else if (ascii_case_equal(command.command, "NICK"))
		handle_nick(fd, command);
	else if (ascii_case_equal(command.command, "USER"))
		handle_user(fd, command);
	else if (ascii_case_equal(command.command, "JOIN"))
		handle_join(fd, command);
	else if (ascii_case_equal(command.command, "PART"))
		handle_part(fd, command);
	else if (ascii_case_equal(command.command, "PRIVMSG"))
		handle_privmsg(fd, command);
	else if (ascii_case_equal(command.command, "TOPIC"))
		handle_topic(fd, command);
	else if (ascii_case_equal(command.command, "KICK"))
		handle_kick(fd, command);
	else if (ascii_case_equal(command.command, "INVITE"))
		handle_invite(fd, command);
	else if (ascii_case_equal(command.command, "MODE"))
		handle_mode(fd, command);
	else if (ascii_case_equal(command.command, "QUIT"))
		handle_quit(fd, command);
}