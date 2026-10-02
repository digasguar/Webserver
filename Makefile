NAME = webserv
BONUS_NAME = webserv_bonus

CXX = g++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -Iinclude -O2

SRCS = src/CgiEnv.cpp \
	   src/CgiProcess.cpp \
	   src/CgiRunner.cpp \
	   src/Client.cpp \
	   src/ConfigLookup.cpp \
	   src/ConfigParser.cpp \
	   src/ConfigTokenizer.cpp \
	   src/multiplexarserver.cpp \
	   src/ProcesRequest.cpp \
	   src/utils.cpp

BONUS_SRCS = bonus/src/CgiEnv.cpp \
			 bonus/src/CgiProcess.cpp \
			 bonus/src/CgiRunner.cpp \
			 bonus/src/Client.cpp \
			 bonus/src/ConfigLookup.cpp \
			 bonus/src/ConfigParser.cpp \
			 bonus/src/ConfigTokenizer.cpp \
			 bonus/src/Cookie.cpp \
			 bonus/src/CookiesManager.cpp \
			 bonus/src/multiplexarserver.cpp \
			 bonus/src/ProcesRequest.cpp \
			 bonus/src/utils.cpp

OBJS = $(SRCS:.cpp=.o)
BONUS_OBJS = $(BONUS_SRCS:.cpp=.o)

RM = rm -f

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

bonus: $(BONUS_NAME)

$(BONUS_NAME): $(BONUS_OBJS)
	$(CXX) $(CXXFLAGS) $(BONUS_OBJS) -o $(BONUS_NAME)

bonus/src/%.o: bonus/src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS) $(BONUS_OBJS)

fclean: clean
	$(RM) $(NAME) $(BONUS_NAME)

re: fclean all

rebonus: fclean bonus

.PHONY: all bonus clean fclean re rebonus
