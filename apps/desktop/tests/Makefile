ifneq ($(wildcard C:/msys64/mingw64/bin/gcc.exe),)
CC := C:/msys64/mingw64/bin/gcc
PKG_CONFIG := C:/msys64/mingw64/bin/pkg-config
else
CC := gcc
PKG_CONFIG := pkg-config
endif

CJSON_CFLAGS := $(shell $(PKG_CONFIG) --cflags libcjson 2>/dev/null)
CJSON_LIBS := $(shell $(PKG_CONFIG) --libs libcjson 2>/dev/null)

CFLAGS = -Wall -Wextra -I../include/ui -I../include/core -I../include/infra $(CJSON_CFLAGS)
LDFLAGS = $(CJSON_LIBS) -lcurl

API_SRC = ../src/core/api.c ../src/infra/config.c ../src/infra/filesystem.c

test_api: test_api.c $(API_SRC)
	$(CC) $(CFLAGS) test_api.c $(API_SRC) -o test_api.exe $(LDFLAGS)

test: test_api
	./test_api.exe

clean:
	rm -f test_api.exe

.PHONY: test clean