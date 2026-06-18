# AURA Professional Packaging System - Quick Start Guide

## For First-Time Users

### Step 1: Build the Application

```bash
cd P-F-A
make clean
make all
```

**Expected Output:**
```
[OK] All .c files compiled successfully
[OK] Object files created in obj/
[OK] bin/AURA.exe created (~5-8 MB)
```

### Step 2: Test First Run

Create a test directory:
```bash
mkdir test_aura
cd test_aura
copy ..\bin\AURA.exe .
```

Run the application:
```bash
AURA.exe
```

**Expected Result:**
1. Splash screen appears
2. Progress bar shows: "Initializing..."
3. Directories auto-created: `data/`, `assets/`, `config/`, `cache/`
4. Configuration file created: `config/aura.cfg`
5. Login screen appears

### Step 3: Verify Directory Structure

After first run, check that this was created:
```
test_aura/
├── AURA.exe
├── data/
│   └── accounts.txt (created if auth_init works)
├── assets/
│   ├── images/
│   ├── sounds/
│   ├── fonts/
│   └── animations/
├── config/
│   └── aura.cfg
└── cache/
```

## For Developers

### Integration Checklist

If you're adding existing code to use the packaging system:

- [ ] Include `launcher.h` in main.c
- [ ] Include `filesystem.h` in modules that access files
- [ ] Include `config.h` in modules that access settings
- [ ] Call `aura_launcher_init_all_systems()` after GTK init
- [ ] Update file paths to use `aura_fs_get_*_path()`
- [ ] Update hardcoded settings to use `aura_config_get_*()`
- [ ] Call `aura_launcher_cleanup_all_systems()` before exit
- [ ] Test with `make clean && make all`

### Testing Your Integration

**Test 1: Verify Paths Resolve**
```c
void test_paths(void) {
    char test_path[1024];
    aura_fs_get_data_path("test.txt", test_path, sizeof(test_path));
    printf("Data path: %s\n", test_path);
    
    // Verify it contains the app directory
    assert(strstr(test_path, "data") != NULL);
}
```

**Test 2: Verify Configuration Loads**
```c
void test_config(void) {
    int width = aura_config_get_int("window_width", -1);
    assert(width > 0);
    printf("Window width from config: %d\n", width);
}
```

**Test 3: Verify Directories Created**
```c
void test_directories(void) {
    assert(aura_fs_dir_exists(g_aura_fs.data_dir));
    assert(aura_fs_dir_exists(g_aura_fs.assets_dir));
    assert(aura_fs_dir_exists(g_aura_fs.config_dir));
    assert(aura_fs_dir_exists(g_aura_fs.cache_dir));
    printf("All directories verified\n");
}
```

## Build Commands Reference

### Full Build Cycle
```bash
make clean    # Remove old build artifacts
make all      # Compile and link
```

### Just Compile
```bash
make all      # Compiles .c files and links executable
```

### Just Clean
```bash
make clean    # Removes obj/ and bin/
```

### Rebuild Specific File
Edit the file, then:
```bash
make all      # Makefile rebuilds only changed files
```

## Project Structure Overview

### New Professional Packaging Components

```
src/
├── launcher.c          [NEW] Main orchestrator
├── filesystem.c        [NEW] Path & directory management
├── assets.c            [NEW] Resource loading
├── config.c            [NEW] Settings management
├── startup.c           [NEW] Initialization sequence
└── main.c              [UPDATED] Uses launcher

include/
├── launcher.h          [NEW]
├── filesystem.h        [NEW]
├── assets.h            [NEW]
├── config.h            [NEW]
├── startup.h           [NEW]
└── (existing headers)

Documentation/
├── PACKAGING_SYSTEM.md [NEW] Technical documentation
├── RELEASE_GUIDE.md    [NEW] Distribution guide
├── DEVELOPER_GUIDE.md  [NEW] Integration instructions
├── ARCHITECTURE.md     [NEW] System design
└── QUICK_START.md      [NEW] This file
```

### Build Output

```
bin/
├── AURA.exe           # Main executable
└── aura_cli.exe       # Symlink/copy for compatibility

obj/
├── launcher.o
├── filesystem.o
├── assets.o
├── config.o
├── startup.o
├── main.o
└── (other .o files)
```

## Common Tasks

### Task: Make a Release Build

```bash
# 1. Clean build
make clean
make all

# 2. Create release folder
mkdir AURA_Release
cd AURA_Release
mkdir data assets config cache
mkdir assets\images assets\sounds assets\fonts assets\animations

# 3. Copy executable
copy ..\bin\AURA.exe AURA.exe

# 4. Test
AURA.exe

# 5. Package
# Now ready to distribute or zip
```

### Task: Debug Path Issues

Add this to any module:
```c
void debug_paths(void) {
    printf("[DEBUG] App root: %s\n", aura_fs_get_app_root());
    printf("[DEBUG] Data dir: %s\n", g_aura_fs.data_dir);
    printf("[DEBUG] Assets dir: %s\n", g_aura_fs.assets_dir);
    printf("[DEBUG] Config dir: %s\n", g_aura_fs.config_dir);
    printf("[DEBUG] Accounts file: %s\n", g_aura_fs.accounts_file);
}

// Call from main() after aura_launcher_init_all_systems()
debug_paths();
```

### Task: Change Default Configuration

Edit `src/config.c`:
```c
static const AuraConfig DEFAULT_CONFIG = {
    .app_name = "My Custom App",
    .window_width = 1920,        // Change this
    .window_height = 1080,       // Change this
    .theme = "light",            // Change this
    // ...
};
```

Then rebuild:
```bash
make clean
make all
```

### Task: Add New Configuration Option

1. Edit `include/config.h` - Add field to `AuraConfig` struct:
```c
typedef struct {
    // ... existing fields ...
    char language[32];           // NEW FIELD
} AuraConfig;
```

2. Edit `src/config.c` - Initialize default:
```c
static const AuraConfig DEFAULT_CONFIG = {
    // ... existing defaults ...
    .language = "en",            // NEW DEFAULT
};
```

3. Edit `src/config.c` - Add to parser:
```c
} else if (strcmp(key, "language") == 0) {
    strncpy(g_aura_config.language, value_start, sizeof(g_aura_config.language) - 1);
```

4. Edit `src/config.c` - Add to saver:
```c
fprintf(file, "language = %s\n", g_aura_config.language);
```

5. Add getter in `config.h`:
```c
const char *aura_config_get_language(void);
```

6. Add getter implementation in `src/config.c`:
```c
const char *aura_config_get_language(void) {
    return g_aura_config.language;
}
```

Rebuild:
```bash
make clean
make all
```

## Environment Setup

### Windows with MSYS2

```bash
# In MSYS2 terminal
pacman -S mingw-w64-x86_64-gtk3 mingw-w64-x86_64-gcc
cd P-F-A
make all
```

### Linux

```bash
sudo apt-get install libgtk-3-dev libcjson-dev libcurl4-openssl-dev libsqlite3-dev
cd P-F-A
make all
```

### macOS

```bash
brew install gtk+3 libcjson curl sqlite
cd P-F-A
make all
```

## Troubleshooting

### Problem: "make: command not found"

**Solution**: Install MinGW/MSYS2 with make
```bash
# Download from mingw-w64 or use package manager
pacman -S mingw-w64-x86_64-make
```

### Problem: "gcc: command not found"

**Solution**: Install GCC compiler
```bash
# Windows (MSYS2)
pacman -S mingw-w64-x86_64-gcc

# Linux
sudo apt-get install build-essential

# macOS
brew install gcc
```

### Problem: "gtk.h: No such file"

**Solution**: Install GTK3 development headers
```bash
# Linux
sudo apt-get install libgtk-3-dev

# macOS
brew install gtk+3

# Windows - Include in MinGW
pacman -S mingw-w64-x86_64-gtk3
```

### Problem: Application crashes at startup

**Solution**: Check initialization output
```bash
# Run from command line to see output
AURA.exe

# If crashes silently, check these:
# 1. GTK3 installed and working
# 2. Permission to create directories
# 3. Working directory is writable
```

### Problem: "Failed to create directory"

**Solution**: Check write permissions
```bash
# Windows: Run as Administrator
# Linux: Check directory permissions
ls -ld /path/to/parent

# Give write permissions if needed
chmod 755 /path/to/parent
```

## Module Reference Quick View

### launcher.h
- `aura_launcher_init_all_systems()`
- `aura_launcher_cleanup_all_systems()`
- `aura_launcher_verify_integrity()`

### filesystem.h
- `aura_fs_init()`
- `aura_fs_get_data_path(...)`
- `aura_fs_get_asset_path(...)`
- `aura_fs_get_config_path(...)`
- `aura_fs_file_exists(...)`
- `aura_fs_dir_exists(...)`

### config.h
- `aura_config_init()`
- `aura_config_get_int("key", default)`
- `aura_config_get_string("key", default)`
- `aura_config_get_bool("key", default)`
- `aura_config_set_int("key", value)`
- `aura_config_set_string("key", value)`
- `aura_config_set_bool("key", value)`

### assets.h
- `aura_assets_init()`
- `aura_assets_load("name", type)`
- `aura_assets_exists("name")`
- `aura_assets_verify_critical()`

### startup.h
- `aura_startup_create_splash()`
- `aura_startup_run_sequence()`
- `aura_startup_update_progress(...)`

## Files Modified

- `src/main.c` - Updated to use launcher
- `Makefile` - Updated executable name

## Files Created

- `src/launcher.c`
- `src/filesystem.c`
- `src/assets.c`
- `src/config.c`
- `src/startup.c`
- `include/launcher.h`
- `include/filesystem.h`
- `include/assets.h`
- `include/config.h`
- `include/startup.h`

## Documentation Created

- `PACKAGING_SYSTEM.md` - Technical overview
- `RELEASE_GUIDE.md` - How to package and distribute
- `DEVELOPER_GUIDE.md` - How to integrate with existing code
- `ARCHITECTURE.md` - System design and internals
- `QUICK_START.md` - This file

## Next Steps

1. **Build**: `make clean && make all`
2. **Test**: Run `bin/AURA.exe` from test directory
3. **Verify**: Check that directories are created
4. **Integrate**: Update existing modules to use new system
5. **Distribute**: Package AURA folder for release

## Support Resources

| Document | Purpose |
|----------|---------|
| PACKAGING_SYSTEM.md | Technical architecture and module docs |
| RELEASE_GUIDE.md | How to create and distribute releases |
| DEVELOPER_GUIDE.md | How to integrate with your code |
| ARCHITECTURE.md | System design and data flow |
| QUICK_START.md | This file - quick reference |

---

**Ready to Build!**

```bash
make clean
make all
./bin/AURA.exe
```

**Professional desktop application packaging complete.**
