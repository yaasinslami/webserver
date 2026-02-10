NAME        :=  webserv
CONTAINER   :=  webserv_dev
CC          :=  c++
CFLAGS      :=  -std=c++98
INCLUDES    :=  -Iinc

SRC_DIR     :=  src
OBJ_DIR     :=  build

SRC_FILES   :=  $(shell find $(SRC_DIR) -type f -name "*.cpp")
OBJ_FILES   :=  $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))

# find all headers (they will be in inc/)
HEADERS     :=  $(shell find inc -type f -name "*.hpp")

# Docker command (use 'docker compose' if you have V2, otherwise 'docker-compose')
D_COMPOSE   := docker-compose

all:    $(NAME)

$(NAME):    $(OBJ_FILES)
	@echo "🔧 Linking $@"
	@$(CC) $(CFLAGS) $(OBJ_FILES) -o $@
	@echo "✅ Built $@"

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

# **************************************************************************** #
#                                   DOCKER                                     #
# **************************************************************************** #

# Build the image and start the container in the background
docker-up:
	@echo "🐳 Building and starting Docker container..."
	@$(D_COMPOSE) up -d --build
	@echo "✅ Container $(CONTAINER) is running!"

# Stop and remove the container
docker-down:
	@echo "🛑 Stopping Docker container..."
	@$(D_COMPOSE) down
	@echo "✅ Container stopped."

# Enter the container shell (Use this to compile/run inside Linux)
docker-shell:
	@echo "💻 Entering container shell..."
	@docker exec -it $(CONTAINER) bash

# View container logs
docker-logs:
	@docker logs -f $(CONTAINER)

# Recompile the project INSIDE the running container
docker-re:
	@echo "🔄 Recompiling inside Docker..."
	@docker exec $(CONTAINER) make re

.PHONY: all clean fclean re docker-up docker-down docker-shell docker-logs docker-re