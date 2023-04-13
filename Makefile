# Variables
CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Wdouble-promotion -Werror=pedantic -Werror=vla -pedantic-errors -Wfatal-errors
DEBUG ?= 0

# Debug mode
ifeq ($(DEBUG), 1)
	CFLAGS += -g -O0
else
	CFLAGS += -O3
endif

# Directories
SRC_DIR := src

# Add here .c dependency files
CLIENT_FILES := tracer.c parser.c utils.c
SERVER_FILES := monitor.c utils.c

OBJ_DIR := obj
CLIENT_OBJ := $(addprefix $(OBJ_DIR)/, $(CLIENT_FILES:.c=.o))
SERVER_OBJ := $(addprefix $(OBJ_DIR)/, $(SERVER_FILES:.c=.o))

BIN_DIR := bin
TMP_DIR := tmp

INC_DIR := include
CFLAGS += -I $(INC_DIR)

all: folders client server

folders:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR) $(TMP_DIR)

# Client
client: folders $(BIN_DIR)/tracer

$(BIN_DIR)/tracer: $(CLIENT_OBJ)
	@$(CC) $(CFLAGS) $^ -o $@;
	@echo " Successfully made tracer (client)"

# Server
server: folders $(BIN_DIR)/monitor

$(BIN_DIR)/monitor: $(SERVER_OBJ)
	@$(CC) $(CFLAGS) $^ -o $@;
	@echo " Successfully made monitor (server)"

# Object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@$(CC) $(CFLAGS) -c $< -o $@;
	@echo " Successfully made object $@"

clean:
	@rm -rf $(OBJ_DIR) $(BIN_DIR) $(TMP_DIR);
	@echo " Successfully cleaned"

rebuild: clean all

format:
	@clang-format --verbose -i $(SRC_DIR)/* $(INC_DIR)/*;
	@echo " Successfully formatted"

check-format:
	@clang-format --dry-run --Werror $(SRC_DIR)/* $(INC_DIR)/*;
	@echo " Successfully checked format"

lint:
	@clang-tidy --warnings-as-errors=* $(SRC_DIR)/* $(INC_DIR)/*;
	@echo " Successfully linted"

# Delete object files if a command fails
.DELETE_ON_ERROR:
