NAME        :=  webserv
CC          :=  c++
CFLAGS      :=  -Wall -Wextra -Werror -std=c++98
INCLUDES    :=  -Iinc

SRC_DIR     :=  src
OBJ_DIR     :=  build

# Headers for dependency tracking
HEADERS     :=  inc/Server.hpp \
                inc/Request.hpp \
                inc/Response.hpp \
                inc/ConfigParser.hpp \
                inc/Directives.hpp \
                inc/Http.hpp \
                inc/Location.hpp \
                inc/CgiHandler.hpp \
                inc/Client.hpp \
                inc/EpollManager.hpp \
                inc/Utils.hpp

# Updated to reflect the logical file structure
SRC_FILES   :=  src/main.cpp \
                src/cgi/CgiHandler.cpp \
                src/config/ConfigParser.cpp \
                src/config/Directives.cpp \
                src/config/Location.cpp \
                src/core/Server.cpp \
                src/core/Client.cpp \
                src/core/EpollManager.cpp \
                src/config/Http.cpp \
                src/http/Request.cpp \
                src/http/Response.cpp \
                src/utils/Utils.cpp

# Pattern substitution to place .o files in the build/ directory
OBJ_FILES   :=  $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_FILES))

all: $(NAME)

$(NAME): $(OBJ_FILES)
	@echo "🔧 Linking $@"
	@$(CC) $(CFLAGS) $(OBJ_FILES) -o $@
	@echo "✅ Built $@"

# This rule creates the necessary subdirectories in build/ automatically
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
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