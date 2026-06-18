ifneq ($(wildcard C:/msys64/mingw64/bin/gcc.exe),)
CC := C:/msys64/mingw64/bin/gcc
PKG_CONFIG := C:/msys64/mingw64/bin/pkg-config
else
CC := gcc
PKG_CONFIG := pkg-config
endif

ifeq ($(OS),Windows_NT)
ifneq ($(MSYSTEM),)
# Keep MSYS-provided shell.
else ifneq ($(wildcard C:/msys64/mingw64/bin/gcc.exe),)
# Use Make default shell when MinGW/MSYS toolchain is detected.
else
SHELL := cmd
endif
endif

ifneq ($(MSYSTEM),)
RUN_TEST_CMD = ./test_api.exe
RM_FILE_CMD = rm -f "$(1)"
else ifneq ($(wildcard C:/msys64/mingw64/bin/gcc.exe),)
RUN_TEST_CMD = ./test_api.exe
RM_FILE_CMD = rm -f "$(1)"
else ifeq ($(OS),Windows_NT)
RUN_TEST_CMD = test_api.exe
RM_FILE_CMD = if exist "$(1)" del /Q "$(1)"
else
RUN_TEST_CMD = ./test_api.exe
RM_FILE_CMD = rm -f "$(1)"
endif

ifneq ($(MSYSTEM),)
DEVNULL := /dev/null
else ifneq ($(wildcard C:/msys64/mingw64/bin/gcc.exe),)
DEVNULL := /dev/null
else ifeq ($(OS),Windows_NT)
DEVNULL := NUL
else
DEVNULL := /dev/null
endif

CJSON_CFLAGS := $(shell $(PKG_CONFIG) --cflags libcjson 2>$(DEVNULL))
CJSON_LIBS := $(shell $(PKG_CONFIG) --libs libcjson 2>$(DEVNULL))

CFLAGS = -Wall -Wextra -I../include/ui -I../include/core -I../include/infra $(CJSON_CFLAGS)
LDFLAGS = $(CJSON_LIBS) -lcurl

API_SRC = ../src/core/api.c ../src/infra/config.c ../src/infra/filesystem.c

test_api: test_api.c $(API_SRC)
	$(CC) $(CFLAGS) test_api.c $(API_SRC) -o test_api.exe $(LDFLAGS)

test: test_api
	$(RUN_TEST_CMD)

clean:
	$(call RM_FILE_CMD,test_api.exe)

.PHONY: test clean