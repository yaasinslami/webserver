NAME		:=	webserv
CC			:=	c++
CFLAGS		:=	-Wall -Wextra -Werror -std=c++11
INCLUDES	:=	-Iinc

SRC_DIR		:=	src
OBJ_DIR		:=	build

SRC_FILES	:=	$(shell find $(SRC_DIR) -type f -name "*.cpp")
OBJ_FILES	:=	$(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))

# find all headers (they will be in inc/)
HEADERS		:=	$(shell find inc -type f -name "*.hpp")

all:	$(NAME)

$(NAME):	$(OBJ_FILES)
	@echo "🔧 Linking $@"
	@$(CC) $(CFLAGS) $(OBJ_FILES) -o $@
	@echo "✅ Built $@"

$(OBJ_DIR)/%.o:	$(SRC_DIR)/%.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	@echo "🧩 Compiling $<"
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	@echo "🧹 Removing object files..."
	@rm -rf $(OBJ_DIR)

fclean: clean
	@echo "🧹 Removing executable..."
	@rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
