NAME		= ircserv
CXX			= c++
CXXFLAGS	= -O3 -Wall -Wextra -Werror -Iinclude -std=c++98 -MMD -MP
RM			= rm -rf

OBJ_DIR		= obj/
SRC_DIR		= src/
FILES		= main.cpp

SRCS		= $(addprefix $(SRC_DIR), $(FILES))
OBJS		= $(addprefix $(OBJ_DIR), $(FILES:.cpp=.o))
DEPS		= $(OBJS:.o=.d)

all:
	@TOTAL=$$($(MAKE) -n $(NAME) 2>/dev/null | grep -c "\[PROGRESS\]"); \
	if [ $$TOTAL -eq 0 ]; then \
		$(MAKE) --no-print-directory $(NAME); \
	else \
		{ $(MAKE) --no-print-directory $(NAME) || echo "[FAIL]"; } | awk -v total=$$TOTAL ' \
		BEGIN { count = 0; } \
		/^\[FAIL\]/ { exit 1; } \
		/^\[PROGRESS\]/ { \
			count++; \
			percent = int(count * 100 / total); \
			printf "\r\033[1;36m[%3d%%]\033[0m Compiling : \033[1;33m%s\033[0m... \033[K", percent, substr($$0, 12); \
			fflush(); \
			next; \
		} \
		{ print $$0; fflush(); } \
		'; \
	fi

$(NAME): $(OBJS)
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@printf "\n\033[1;32m[✓] Compilation done '$(NAME)' is ready\033[0m\n"

$(OBJ_DIR)%.o: $(SRC_DIR)%.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@
	@echo "[PROGRESS] $<"

-include $(DEPS)

clean:
	@$(RM) $(OBJ_DIR)
	@printf "\033[1;90m[✓] Cleaning obj (.o et .d).\033[0m\n"

fclean: clean
	@$(RM) $(NAME)
	@printf "\033[1;90m[✓] Binary deleted.\033[0m\n"

re: fclean all
.PHONY: all clean fclean re