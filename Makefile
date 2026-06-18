ifneq ($(wildcard C:/msys64/mingw64/bin/gcc.exe),)
CC := C:/msys64/mingw64/bin/gcc
PKG_CONFIG := C:/msys64/mingw64/bin/pkg-config
SHELL := C:/msys64/usr/bin/bash.exe
else
CC := gcc
PKG_CONFIG := pkg-config
endif

GTK_CFLAGS := $(shell $(PKG_CONFIG) --cflags gtk+-3.0)
CJSON_CFLAGS := $(shell $(PKG_CONFIG) --cflags libcjson)
GTK_LIBS := $(shell $(PKG_CONFIG) --libs gtk+-3.0)
CJSON_LIBS := $(shell $(PKG_CONFIG) --libs libcjson)

CFLAGS = -Wall -Wextra -Iinclude $(GTK_CFLAGS) $(CJSON_CFLAGS)
LDFLAGS = $(GTK_LIBS) $(CJSON_LIBS) -lcurl -lsqlite3

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
DATA_DIR = data
MSYS2_BIN = C:/msys64/mingw64/bin

RUNTIME_DLLS = \
libatk-1.0-0.dll \
libcairo-2.dll \
libcairo-gobject-2.dll \
libcurl-4.dll \
libexpat-1.dll \
libffi-8.dll \
libfontconfig-1.dll \
libfreetype-6.dll \
libgdk-3-0.dll \
libgdk_pixbuf-2.0-0.dll \
libgio-2.0-0.dll \
libglib-2.0-0.dll \
libgmodule-2.0-0.dll \
libgobject-2.0-0.dll \
libgraphite2.dll \
libgtk-3-0.dll \
libharfbuzz-0.dll \
libiconv-2.dll \
libintl-8.dll \
libpango-1.0-0.dll \
libpangocairo-1.0-0.dll \
libpangowin32-1.0-0.dll \
libpcre2-8-0.dll \
libpixman-1-0.dll \
libpng16-16.dll \
libsqlite3-0.dll \
libwinpthread-1.dll \
zlib1.dll

# Primary executable name (user-facing)
TARGET = $(BIN_DIR)/AURA.exe
# Alternative for backward compatibility
ALT_TARGET = $(BIN_DIR)/aura_cli.exe

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

all: $(OBJ_DIR) $(BIN_DIR) $(DATA_DIR) $(TARGET)

ifneq ($(wildcard C:/msys64/mingw64/bin/gcc.exe),)
MKDIR_CMD = mkdir -p "$(1)"
RM_CMD = rm -rf "$(1)"
else ifeq ($(OS),Windows_NT)
MKDIR_CMD = if not exist "$(1)" mkdir "$(1)"
RM_CMD = if exist "$(1)" rmdir /S /Q "$(1)"
else
MKDIR_CMD = mkdir -p "$(1)"
RM_CMD = rm -rf "$(1)"
endif

$(OBJ_DIR):
	$(call MKDIR_CMD,$@)

$(BIN_DIR):
	$(call MKDIR_CMD,$@)

$(DATA_DIR):
	$(call MKDIR_CMD,$@)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(call RM_CMD,$(OBJ_DIR))
	$(call RM_CMD,$(BIN_DIR))
	$(call MKDIR_CMD,$(OBJ_DIR))
	$(call MKDIR_CMD,$(BIN_DIR))

.PHONY: all clean

# Release packaging
RELEASE_DIR = AURA
RELEASE_ASSETS = $(RELEASE_DIR)/assets
RELEASE_DATA = $(RELEASE_DIR)/data
RELEASE_CONFIG = $(RELEASE_DIR)/config
RELEASE_CACHE = $(RELEASE_DIR)/cache
RELEASE_LOGS = $(RELEASE_DIR)/logs

ICON_FILE = rsc/aura.ico

.PHONY: release release-win package

release: release-win

release-win:
	powershell -ExecutionPolicy Bypass -File build.ps1
	powershell -ExecutionPolicy Bypass -File scripts/package_release.ps1

package: $(TARGET)
	@echo Preparing release folder '$(RELEASE_DIR)'
	@if exist "$(RELEASE_DIR)" rmdir /S /Q "$(RELEASE_DIR)" || echo ""
	@mkdir "$(RELEASE_DIR)"
	@mkdir "$(RELEASE_ASSETS)"
	@mkdir "$(RELEASE_ASSETS)\images"
	@mkdir "$(RELEASE_ASSETS)\sounds"
	@mkdir "$(RELEASE_ASSETS)\videos"
	@mkdir "$(RELEASE_ASSETS)\fonts"
	@mkdir "$(RELEASE_ASSETS)\animations"
	@mkdir "$(RELEASE_DATA)"
	@mkdir "$(RELEASE_CONFIG)"
	@mkdir "$(RELEASE_CACHE)"
	@mkdir "$(RELEASE_LOGS)"

	@echo Copying binary to release folder
	@copy /Y "$(TARGET)" "$(RELEASE_DIR)\AURA.exe" > NUL
	@copy /Y "$(BIN_DIR)\*.dll" "$(RELEASE_DIR)" > NUL 2>&1

	@echo Copying data files
	@if exist "$(DATA_DIR)\accounts.txt" copy /Y "$(DATA_DIR)\accounts.txt" "$(RELEASE_DATA)\accounts.txt" > NUL || echo "accounts.txt not found"
	@copy /Y release_templates\scores.txt "$(RELEASE_DATA)\scores.txt" > NUL
	@copy /Y release_templates\reports.txt "$(RELEASE_DATA)\reports.txt" > NUL
	@copy /Y release_templates\questions.txt "$(RELEASE_DATA)\questions.txt" > NUL

	@echo Copying config
	@copy /Y release_templates\aura_config.cfg "$(RELEASE_CONFIG)\aura_config.cfg" > NUL
	@copy /Y release_templates\aura_config.cfg "$(RELEASE_CONFIG)\aura.cfg" > NUL

	@echo Copying placeholder assets
	@xcopy /E /I /Y bin\assets "$(RELEASE_ASSETS)" > NUL 2>&1 || (
		echo No prebuilt assets found in bin\\assets, copying placeholders
		copy /Y release_templates\assets\images\logo.png "$(RELEASE_ASSETS)\images\logo.png" > NUL
		copy /Y release_templates\assets\sounds\intro.mp3 "$(RELEASE_ASSETS)\sounds\intro.mp3" > NUL
		copy /Y release_templates\assets\videos\intro.mp4 "$(RELEASE_ASSETS)\videos\intro.mp4" > NUL
		copy /Y release_templates\assets\fonts\Inter-Regular.ttf "$(RELEASE_ASSETS)\fonts\Inter-Regular.ttf" > NUL
		copy /Y release_templates\assets\animations\intro.anim "$(RELEASE_ASSETS)\animations\intro.anim" > NUL
	)

	@echo Release ready in '$(RELEASE_DIR)'
	@echo Double-click $(RELEASE_DIR)\\AURA.exe to launch

