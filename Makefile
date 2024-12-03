RED		:=	\e[31m
GREEN	:=	\e[32m
ORANGE	:=	\e[33m
BLUE	:=	\e[34m
PURPLE	:=	\e[35m
CYAN	:=	\e[36m

BOLD	:=	\e[1m
DIM		:=	\e[2m
NORMAL	:=	\e[22m
ITA		:=	\e[3m
NOITA	:=	\e[23m

BEGIN	:=	\e[1A\e[K
DEFAULT	:=	\e(B\e[m

FILES	:=	main.cpp \
			Server.cpp

SRC_DIR	:=	src
OBJ_DIR	:=	obj
SRC		:=	$(addprefix $(SRC_DIR)/, $(FILES))
OBJ		:=	$(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC))

DEP		:=	$(OBJ:.o=.d)

CXX			:=	c++
CXXFLAGS	:=	-Wall -Wextra -Werror -std=c++98 -I inc/
LDFLAGS		:= 

NAME		:=	webserv

.PHONY: all release

all: $(NAME)

release: CXXFLAGS += -O3
release: 
	@echo "$(BLUE)$(ITA)$(CXX)$(NOITA) $(DIM)$(CXXFLAGS) $(LDFLAGS)$(NORMAL) -o $(BOLD)$(NAME)$(NORMAL) $(SRC)$(DEFAULT)"
	@$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(NAME) $(SRC)

NOVALGRIND	:= -fsanitize=address
noval: CXXFLAGS += $(NOVALGRIND)
noval: LDFLAGS += $(NOVALGRIND)
noval: $(NAME)

$(NAME): CXXFLAGS += -g3 -D_GLIBCXX_DEBUG -DDEBUG
$(NAME): $(OBJ)
	@echo "$(BLUE)$(ITA)$(CXX)$(NOITA) $(DIM)$(LDFLAGS)$(NORMAL) -o $(BOLD)$@$(NORMAL) $^$(DEFAULT)"
	@$(CXX) $(LDFLAGS) -o $@ $^


-include $(DEP)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp Makefile | $(OBJ_DIR)
	@echo "$(ORANGE) $(ITA)Compiling$(NOITA) $(BOLD)$<$(NORMAL) with \"$(DIM)$(CXXFLAGS) -MMD -MP$(NORMAL)\"$(DEFAULT)"
	@$(CXX) $(CXXFLAGS) -MMD -MP -o $@ -c $< && \
		echo "$(BEGIN)$(GREEN)Successfully $(ITA)compiled$(NOITA) with \"$(DIM)$(CXXFLAGS) -MMD -MP$(NORMAL)\" into $(BOLD)$@$(NORMAL)$(DEFAULT)"

$(OBJ_DIR):
	@echo "$(DIM)$(ITA)mkdir$(NOITA) -p $@$(DEFAULT)"
	@mkdir -p $@

.PHONY: clean fclean re
clean:
	@echo "$(RED)$(ITA)rm$(NOITA) -rf $(BOLD)./$(OBJ_DIR)$(DEFAULT)"
	@rm -rf ./$(OBJ_DIR)

fclean: clean
	@echo "$(RED)$(ITA)rm$(NOITA) -f $(BOLD)$(NAME)$(DEFAULT)"
	@rm -f $(NAME)

re: fclean all

