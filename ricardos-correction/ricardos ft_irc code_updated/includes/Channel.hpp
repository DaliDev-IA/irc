#ifndef CHANNEL_HPP
# define CHANNEL_HPP

# include <set>
# include <string>

// users are stored by their socket fd
class Channel
{
	public:
		std::string		name;
		std::string		topic;
		std::string		key;		// mode k (empty = no key)
		int				limit;		// mode l (0 = no limit)
		bool			inviteOnly;	// mode i
		bool			topicOps;	// mode t
		std::set<int>	members;
		std::set<int>	ops;
		std::set<int>	invited;

		Channel();

		bool		has(int fd) const;
		bool		isOp(int fd) const;
		std::string	modes() const;	// "+itk" for MODE #chan
};

#endif
