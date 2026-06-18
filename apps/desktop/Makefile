ifneq ($(wildcard C:/msys64/mingw64/bin/gcc.exe),)
CC := C:/msys64/mingw64/bin/gcc
PKG_CONFIG := C:/msys64/mingw64/bin/pkg-config
SHELL := C:/msys64/usr/bin/bash.exe
else
CC := gcc
PKG_CONFIG := pkg-config
endif

INC_UI = include/ui
INC_CORE = include/core
INC_INFRA = include/infra

GTK_CFLAGS := $(shell $(PKG_CONFIG) --cflags gtk+-3.0)
CJSON_CFLAGS := $(shell $(PKG_CONFIG) --cflags libcjson)
GTK_LIBS := $(shell $(PKG_CONFIG) --libs gtk+-3.0)
CJSON_LIBS := $(shell $(PKG_CONFIG) --libs libcjson)

CFLAGS = -Wall -Wextra -I$(INC_UI) -I$(INC_CORE) -I$(INC_INFRA) $(GTK_CFLAGS) $(CJSON_CFLAGS)
LDFLAGS = $(GTK_LIBS) $(CJSON_LIBS) -lcurl -lsqlite3

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
TARGET = $(BIN_DIR)/AURA.exe

SRCS = src/main.c \
       $(wildcard src/ui/*.c) \
       $(wildcard src/core/*.c) \
       $(wildcard src/infra/*.c)

OBJS = $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRCS))

ifneq ($(MSYSTEM),)
MKDIR_CMD = mkdir -p "$(1)"
RM_CMD = rm -rf "$(1)"
else ifneq ($(wildcard C:/msys64/mingw64/bin/gcc.exe),)
MKDIR_CMD = mkdir -p "$(1)"
RM_CMD = rm -rf "$(1)"
else ifeq ($(OS),Windows_NT)
MKDIR_CMD = if not exist "$(1)" mkdir "$(1)"
RM_CMD = if exist "$(1)" rmdir /S /Q "$(1)"
else
MKDIR_CMD = mkdir -p "$(1)"
RM_CMD = rm -rf "$(1)"
endif

all: $(OBJ_DIR) $(BIN_DIR) $(TARGET)

$(OBJ_DIR):
	$(call MKDIR_CMD,$@)
	$(call MKDIR_CMD,$(OBJ_DIR)/ui)
	$(call MKDIR_CMD,$(OBJ_DIR)/core)
	$(call MKDIR_CMD,$(OBJ_DIR)/infra)

$(BIN_DIR):
	$(call MKDIR_CMD,$@)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: %.c
	$(call MKDIR_CMD,$(dir $@))
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(call RM_CMD,$(OBJ_DIR))
	$(call RM_CMD,$(BIN_DIR))

.PHONY: all clean test

test:
	$(MAKE) -C tests test