SHELL := cmd

ifneq ($(wildcard C:/msys64/mingw64/bin/gcc.exe),)
CC ?= C:/msys64/mingw64/bin/gcc
else
CC ?= gcc
endif

ifneq ($(wildcard C:/msys64/mingw64/bin/pkg-config.exe),)
PKG_CONFIG ?= C:/msys64/mingw64/bin/pkg-config
else
PKG_CONFIG ?= pkg-config
endif

CFLAGS = -Wall -Wextra -Iinclude $(shell $(PKG_CONFIG) --cflags libcjson libcurl sqlite3)
LDFLAGS = $(shell $(PKG_CONFIG) --static --libs libcjson libcurl sqlite3) -lws2_32

SRC_DIR = src
OBJ_DIR = obj
TARGET = server

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

ifeq ($(OS),Windows_NT)
MKDIR_CMD = if not exist "$(1)" mkdir "$(1)"
RM_FILE_CMD = if exist "$(1)" del /Q "$(1)"
RM_DIR_CMD = if exist "$(1)" rmdir /S /Q "$(1)"
else
MKDIR_CMD = mkdir -p "$(1)"
RM_FILE_CMD = rm -f "$(1)"
RM_DIR_CMD = rm -rf "$(1)"
endif

all: $(OBJ_DIR) $(TARGET)

$(OBJ_DIR):
	$(call MKDIR_CMD,$@)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(call RM_DIR_CMD,$(OBJ_DIR))
	$(call RM_FILE_CMD,$(TARGET))
	$(call RM_FILE_CMD,$(TARGET).exe)

.PHONY: all clean
