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
OBJ_DIR := obj
BIN_DIR := bin
TMP_DIR := tmp
INC_DIR := include
CFLAGS += -I $(INC_DIR)

all: folders client server

folders:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR) $(TMP_DIR)

# Client
client: folders $(BIN_DIR)/tracer

$(BIN_DIR)/tracer: $(OBJ_DIR)/tracer.o
	@$(CC) $(CFLAGS) $< -o $@;
	@echo " Successfully made tracer (client)"

$(OBJ_DIR)/tracer.o: $(SRC_DIR)/tracer.c
	@$(CC) $(CFLAGS) -c $< -o $@

# Server
server: folders $(BIN_DIR)/monitor

$(BIN_DIR)/monitor: $(OBJ_DIR)/monitor.o
	@$(CC) $(CFLAGS) $< -o $@;
	@echo " Successfully made monitor (server)"


$(OBJ_DIR)/monitor.o: $(SRC_DIR)/monitor.c
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -rf $(OBJ_DIR) $(BIN_DIR) $(TMP_DIR);
	@echo " Successfully cleaned"

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
