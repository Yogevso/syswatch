CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -O2 -Iinclude
LDFLAGS =

SRC_DIR = src
OBJ_DIR = obj
BIN     = syswatch

SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/process.c \
       $(SRC_DIR)/network.c \
       $(SRC_DIR)/alerts.c

OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

.PHONY: all clean

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(BIN)
