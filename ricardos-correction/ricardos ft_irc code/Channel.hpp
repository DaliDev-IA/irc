#ifndef CHANNEL_HPP
# define CHANNEL_HPP

# include <set>
# include <string>

// One channel. Users are remembered by their socket fd.
class Channel
{
	public:
		std::string		name;
		std::string		topic;
		std::string		key;			// empty = no password   (mode k)
		size_t			limit;			// 0     = no user limit (mode l)
		bool			inviteOnly;		//                       (mode i)
		bool			topicLocked;	// only operators change the topic (mode t)
		std::set<int>	members;
		std::set<int>	operators;
		std::set<int>	invited;

		Channel();

		bool		has(int fd) const;
		bool		isOperator(int fd) const;
		void		remove(int fd);
		std::string	modes() const;		// for example "+itk"
};

#endif
