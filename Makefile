NAME = webserv
BONUS_NAME = webserv_bonus

CXX = g++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -Iinclude -O3

SRCS = $(shell find src -name "*.cpp")
OBJS = $(SRCS:.cpp=.o)

BONUS_SRCS = $(shell find bonus -name "*.cpp")
BONUS_OBJS = $(BONUS_SRCS:.cpp=.o)

RM = rm -f

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

bonus: $(BONUS_NAME)

$(BONUS_NAME): $(BONUS_OBJS)
	$(CXX) $(CXXFLAGS) $(BONUS_OBJS) -o $(BONUS_NAME)

clean:
	$(RM) $(OBJS) $(BONUS_OBJS)

fclean: clean
	$(RM) $(NAME) $(BONUS_NAME)

re: fclean all

.PHONY: all bonus clean fclean re