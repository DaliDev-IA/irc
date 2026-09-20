#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <vector>

class Channel
{
	private:
		std::string			_name;
		std::string			_topic;
		std::string			_key;
		unsigned int		_userLimit;

		std::vector<int>	_members;
		std::vector<int>	_operators;
		std::vector<int>	_invited;

		bool				_inviteOnly;
		bool				_topicRestricted;

	public:
		Channel(const std::string &name);

		const std::string		&get_name() const;
		const std::string		&get_topic() const;
		const std::vector<int>	&get_members() const;

		bool	has_topic() const;
		void	set_topic(const std::string &topic);

		bool	is_empty() const;
		bool	has_member(int fd) const;
		void	add_member(int fd);
		void	remove_member(int fd);

		bool	is_operator(int fd) const;
		void	add_operator(int fd);
		void	remove_operator(int fd);

		bool	is_invited(int fd) const;
		void	add_invited(int fd);
		void	remove_invited(int fd);

		bool	is_invite_only() const;
		void	set_invite_only(bool value);

		bool	is_topic_restricted() const;
		void	set_topic_restricted(bool value);

		bool				has_key() const;
		const std::string	&get_key() const;
		void				set_key(const std::string &key);
		void				remove_key();

		bool			has_user_limit() const;
		unsigned int	get_user_limit() const;
		void			set_user_limit(unsigned int limit);
		void			remove_user_limit();
};

#endif