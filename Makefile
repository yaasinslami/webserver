# Makefile for webserv
NAME        := webserv
CXX         := c++
# change the std if you prefer c++98 / c++11 / c++20
CXXFLAGS    := -Wall -Wextra -Werror -std=c++11
INCLUDES    := -Iinc

SRC_DIR     := src
OBJ_DIR     := build

# find all .cpp files recursively under src/
SRC_FILES   := $(shell find $(SRC_DIR) -type f -name "*.cpp")
# map src/.../file.cpp -> build/.../file.o
OBJ_FILES   := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))

.PHONY: all clean fclean re

all: $(NAME)

$(NAME): $(OBJ_FILES)
	@echo "🔧 Linking $@"
	@$(CXX) $(CXXFLAGS) $(OBJ_FILES) -o $@
	@echo "✅ Built $@"

# compile rule: ensure target dir exists
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "🧩 Compiling $<"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	@echo "🧹 Removing object files..."
	@rm -rf $(OBJ_DIR)

fclean: clean
	@echo "🧹 Removing executable..."
	@rm -f $(NAME)

re: fclean all

