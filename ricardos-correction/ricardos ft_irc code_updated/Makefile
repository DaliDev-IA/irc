NAME		= ircserv

CXX			= c++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98 -Iincludes

SRCS		= srcs/main.cpp \
			  srcs/Server.cpp \
			  srcs/Utils.cpp \
			  srcs/Commands.cpp \
			  srcs/Operators.cpp \
			  srcs/Client.cpp \
			  srcs/Channel.cpp

OBJS		= $(SRCS:srcs/%.cpp=objs/%.o)
HEADERS		= includes/Server.hpp includes/Client.hpp includes/Channel.hpp

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

objs/%.o: srcs/%.cpp $(HEADERS)
	@mkdir -p objs
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf objs

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
