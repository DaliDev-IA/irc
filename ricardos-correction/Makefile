NAME = ircserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

SRC_DIR = srcs
INC_DIR = includes
OBJ_DIR = obj

SRCS =	main.cpp \
		Server.cpp \
		ServerNetwork.cpp \
		ServerCommands.cpp \
		ServerRegistration.cpp \
		ServerProtocol.cpp \
		ServerJoinPart.cpp \
		ServerTopicKick.cpp \
		ServerInvite.cpp \
		ServerMessaging.cpp \
		ServerWho.cpp \
		ServerDisconnect.cpp \
		ServerMode.cpp \
		ServerModeParser.cpp \
		ServerModeSimple.cpp \
		ServerModeParam.cpp \
		ServerModeOperator.cpp \
		ServerTextUtils.cpp \
		ServerClientUtils.cpp \
		ServerChannelUtils.cpp \
		Client.cpp \
		Parser.cpp \
		Channel.cpp

OBJS = $(SRCS:%.cpp=$(OBJ_DIR)/%.o)

HEADERS =	$(INC_DIR)/Server.hpp \
			$(INC_DIR)/Client.hpp \
			$(INC_DIR)/Parser.hpp \
			$(INC_DIR)/Channel.hpp

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -I$(INC_DIR) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re