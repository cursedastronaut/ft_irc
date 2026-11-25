NAME			= ircserv

TEST_NET		= net_test
TEST_PROTO		= proto_test
TEST_CHAN		= chan_test

TEST_ALL		= all_tests

TEST_NET_MEM	= net_mem_test
TEST_PROTO_MEM	= proto_mem_test
TEST_CHA_MEM	= chan_mem_test

TEST_MEMORY		= all_mem_tests

CXX			= c++
CXXFLAGS	= -O3 -Wall -Wextra -Werror -Iinclude -Isrc -std=c++98 -MMD -MP
RM			= rm -rf

OBJ_DIR		= obj/
SRC_DIR		= src/
FILES		= main.cpp\
			net/ClientRecord.cpp\
			net/ScopedShutdownGuard.cpp\
			net/parsers.cpp\
			net/ScopedSocket.cpp\
			net/ServerConfig.cpp\
			protocol/Parser.cpp\
			protocol/ClientRegistry.cpp\
			protocol/Reply.cpp\
			protocol/Dispatcher.cpp\
			protocol/DispatchProtocol.cpp\
			protocol/Guards.cpp\
			commands/auth/PassHandler.cpp\
			commands/auth/NickHandler.cpp\
			commands/auth/UserHandler.cpp\
			channels/Channel.cpp\
			channels/ChannelName.cpp\
			channels/ChannelRegistry.cpp\
			commands/channel/ChannelCommand.cpp\
			commands/channel/Join.cpp\
			commands/channel/Mode.cpp\
			commands/channel/Part.cpp\
			commands/channel/Topic.cpp\
			commands/channel/Kick.cpp\
			commands/channel/ChannelMessage.cpp\
			commands/channel/Invite.cpp

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

$(TEST_NET):
	@echo "<<<<<    NORMAL TEST    >>>>>"
	@echo "<<<<<<<<<    NET    >>>>>>>>>"
	@python3 tests/net/run.py && echo "<<<<<<   NET PASS    >>>>>>>>" || echo "<<<<<<   NET FAIL   >>>>>>>>"

$(TEST_AUTH):
	@echo "<<<<<    NORMAL TEST    >>>>>"
	@echo "<<<<<<    PROTOCOL    >>>>>>>"
	@python3 tests/protocol/run.py && echo "<<<<    PROTOCOL PASS    >>>>" || echo "<<<<    PROTOCOL FAIL    >>>>"

$(TEST_CHAN):
	@echo "<<<<<    NORMAL TEST    >>>>>"
	@echo "<<<<<<    CHANNELS    >>>>>>>"
	@python3 tests/channels/run.py && echo "<<<<    CHANNELS PASS   >>>>>" || echo "<<<<    CHANNELS  FAIL   >>>>>"

$(TEST_ALL): $(TEST_NET) $(TEST_CHAN)


$(TEST_NET_MEM):
	@echo "<<<<<    MEMORY TEST    >>>>>"
	@echo "<<<<<<<<<    NET    >>>>>>>>>"
	@python3 tests/net/run.py --sanitizers \
	&& echo "<<<<<   NET MEMM PASS   >>>>>" || echo "<<<<   NET MEMM FAIL    >>>>>>"

$(TEST_PROTO_MEM):
	@echo "<<<<<    MEMORY TEST    >>>>>"
	@echo "<<<<<<    PROTOCOL    >>>>>>>"
	@python3 tests/protocol/run.py --sanitizers \
	&& echo "<<    PROTOCOL MEMM PASS   >>" || echo "<<    PROTOCOL MEMM FAIL    >>"

$(TEST_CHA_MEM):
	@echo "<<<<<    MEMORY TEST    >>>>>"
	@echo "<<<<<<    CHANNELS    >>>>>>>"
	@python3 tests/channels/run.py --sanitizers \
	&& echo "<<    CHANNELS MEMM PASS   >>" || echo "<<    CHANNELS MEMM FAIL   >>>"

$(TEST_MEMORY):
# 	NET MEMORY TESTS
	@echo "<<<   ALL MEMORY TESTS    >>>"
	@make --no-print-directory $(TEST_NET_MEM)
#	PROTOCOL MEMORY TESTS
	@make --no-print-directory $(TEST_PROTO_MEM)
# 	CHANNELS MEMORY TESTS
	@make --no-print-directory $(TEST_CHA_MEM)
	@echo "<<  ALL MEMORY TESTS PASS  >>"

clean:
	@$(RM) $(OBJ_DIR)
	@printf "\033[1;90m[✓] Cleaning obj (.o et .d).\033[0m\n"

fclean: clean
	@$(RM) $(NAME)
	@printf "\033[1;90m[✓] Binary deleted.\033[0m\n"

re: fclean all
.PHONY: all clean fclean re $(TEST_NET) $(TEST_CHAN) $(TEST_ALL) $(TEST_NET_MEM) $(TEST_CHA_MEM) $(TEST_MEMORY)
