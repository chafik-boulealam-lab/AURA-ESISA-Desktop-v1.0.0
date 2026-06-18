# Developer Integration Guide: AURA Packaging System

## For Existing AURA Developers

This guide explains how to integrate the new professional packaging system into existing AURA code.

## Quick Integration Checklist

- [ ] Include new headers in your modules
- [ ] Update file access to use filesystem module
- [ ] Use configuration for user preferences
- [ ] Initialize launcher at application start
- [ ] Call cleanup at application exit
- [ ] Test with new directory structure
- [ ] Verify asset loading works
- [ ] Check configuration persistence

## Header Files to Include

In modules that access files:

```c
#include "launcher.h"      // Main orchestrator
#include "filesystem.h"    // Path management
#include "config.h"        // Settings access
#include "assets.h"        // Asset loading
```

In main.c specifically:

```c
#include "launcher.h"
#include "filesystem.h"
#include "config.h"
```

## Migration Guide

### Updating File Access

#### Pattern 1: Direct File Path

**Before**:
```c
FILE *file = fopen("data/accounts.txt", "r");
if (file == NULL) {
    fprintf(stderr, "Error: Cannot open accounts.txt\n");
    return false;
}
```

**After**:
```c
char accounts_path[1024];
aura_fs_get_data_path("accounts.txt", accounts_path, sizeof(accounts_path));
FILE *file = fopen(accounts_path, "r");
if (file == NULL) {
    fprintf(stderr, "Error: Cannot open %s\n", accounts_path);
    return false;
}
```

#### Pattern 2: Pre-initialized Path

After `aura_launcher_init_all_systems()`, you can use:

```c
// Global filesystem structure is available
FILE *file = fopen(g_aura_fs.accounts_file, "r");
```

#### Pattern 3: Configuration File

**Before**:
```c
#define DEFAULT_WIDTH 1280
#define DEFAULT_HEIGHT 800
```

**After**:
```c
int width = aura_config_get_int("window_width", 1280);
int height = aura_config_get_int("window_height", 800);
```

### Updating Database Access

#### auth.c Changes

The auth module now receives proper path:

**Before** (in main.c):
```c
auth_init(NULL);  // Searches in current directory
```

**After** (in main.c):
```c
char accounts_path[1024];
aura_fs_get_data_path("accounts.txt", accounts_path, sizeof(accounts_path));
auth_init(accounts_path);  // Uses proper path
```

#### db.c Changes

If db.c writes data files:

```c
// In db.c initialization
char db_path[1024];
aura_fs_get_data_path("database.db", db_path, sizeof(db_path));

// Later when saving
sqlite3 *db;
sqlite3_open(db_path, &db);
```

### Updating Asset Access

#### For Images

**Before**:
```c
GtkWidget *image = gtk_image_new_from_file("assets/images/logo.png");
```

**After**:
```c
char logo_path[1024];
aura_fs_get_asset_path("images/logo.png", logo_path, sizeof(logo_path));
GtkWidget *image = gtk_image_new_from_file(logo_path);
```

#### For Fonts

**Before**:
```c
PangoFontDescription *font = pango_font_description_from_string("assets/fonts/custom.ttf");
```

**After**:
```c
char font_path[1024];
aura_fs_get_asset_path("fonts/custom.ttf", font_path, sizeof(font_path));
PangoFontDescription *font = pango_font_description_from_string(font_path);
```

### Updating Settings Access

#### Using Configuration Values

**Before**:
```c
// Hardcoded values
int window_width = 1280;
int window_height = 800;
bool fullscreen = false;
```

**After**:
```c
// From configuration
int window_width = aura_config_get_int("window_width", 1280);
int window_height = aura_config_get_int("window_height", 800);
bool fullscreen = aura_config_get_bool("start_fullscreen", false);
```

#### Saving User Preferences

**Before**:
```c
// Settings lost on restart
user_theme = "dark";
```

**After**:
```c
// Settings automatically saved
aura_config_set_string("theme", "dark");
// At shutdown, automatically saved via aura_launcher_cleanup_all_systems()
```

## Module Integration Examples

### Integration in auth.c

```c
#include "filesystem.h"

static const char *accounts_path = NULL;

bool auth_init(const char *accounts_path_arg) {
    if (accounts_path_arg != NULL) {
        accounts_path = accounts_path_arg;
    } else {
        // Fallback to default location
        static char default_path[1024];
        aura_fs_get_data_path("accounts.txt", default_path, sizeof(default_path));
        accounts_path = default_path;
    }
    
    // Create file if doesn't exist
    FILE *file = fopen(accounts_path, "a");
    if (file != NULL) {
        fclose(file);
    }
    
    return true;
}

const char *auth_get_accounts_path(void) {
    return accounts_path;
}
```

### Integration in db.c

```c
#include "filesystem.h"

static sqlite3 *g_database = NULL;

void init_db(void) {
    char db_path[1024];
    aura_fs_get_data_path("aura.db", db_path, sizeof(db_path));
    
    int rc = sqlite3_open(db_path, &g_database);
    if (rc) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(g_database));
        return;
    }
    
    printf("Database opened: %s\n", db_path);
    
    // Create tables if needed
    create_tables();
}
```

### Integration in dashboard_ui.c

```c
#include "config.h"

void create_dashboard_window(int *argc, char **argv[]) {
    // Get saved window dimensions
    int width = aura_config_get_int("window_width", 1280);
    int height = aura_config_get_int("window_height", 800);
    
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), width, height);
    
    // Save dimensions on window resize (optional)
    g_signal_connect(window, "configure-event", 
        G_CALLBACK(on_window_configure), NULL);
}

static gboolean on_window_configure(GtkWidget *widget, 
    GdkEventConfigure *event, gpointer user_data) {
    aura_config_set_int("window_width", event->width);
    aura_config_set_int("window_height", event->height);
    return FALSE;
}
```

## Compilation with New Modules

The Makefile automatically includes the new modules:

```bash
# All .c files in src/ are compiled
make clean
make all
```

Generated object files:
- `obj/launcher.o`
- `obj/filesystem.o`
- `obj/assets.o`
- `obj/config.o`
- `obj/startup.o`
- `obj/main.o`
- ... (existing modules)

Final executable: `bin/AURA.exe`

## Testing the Integration

### Test 1: First Run Initialization

```bash
# Create test directory
mkdir test_run
cd test_run

# Copy executable
copy ..\bin\AURA.exe .

# Run
AURA.exe

# Verify directories created:
# - data/
# - assets/ (with subdirs)
# - config/ (with aura.cfg)
# - cache/
```

### Test 2: Configuration Persistence

```bash
# Edit config/aura.cfg manually
# Change window_width to 1920

# Run application
AURA.exe

# Application should use 1920x1200 (or your changes)
```

### Test 3: Path Resolution

Add debug output to test paths:

```c
void test_paths(void) {
    printf("App root: %s\n", aura_fs_get_app_root());
    printf("Accounts: %s\n", g_aura_fs.accounts_file);
    printf("Config: %s\n", g_aura_fs.config_dir);
    printf("Assets: %s\n", g_aura_fs.assets_dir);
}
```

Call from main after `aura_launcher_init_all_systems()`.

### Test 4: Asset Loading

```c
void test_assets(void) {
    // Create test asset
    char img_path[1024];
    aura_fs_get_asset_path("images/test.png", img_path, sizeof(img_path));
    printf("Asset path: %s\n", img_path);
    printf("Exists: %s\n", aura_assets_exists("images/test.png") ? "yes" : "no");
}
```

## Debugging

### Enable Verbose Logging

The modules print to stdout/stderr by default:

```
[AURA] Filesystem initialized
[AURA] App root: C:\Users\mlap\OneDrive\Desktop\P-F-A\bin
[AURA] Assets module initialized...
[AURA] Configuration loaded
```

### Common Issues

**Issue**: "Failed to create directory"

Solution in code:
```c
if (!aura_fs_verify_structure()) {
    fprintf(stderr, "Directory structure verification failed\n");
    // Handle gracefully, don't exit
}
```

**Issue**: "Asset not found"

Solution in code:
```c
if (!aura_assets_exists("images/logo.png")) {
    fprintf(stderr, "Warning: Logo asset missing, using fallback\n");
    // Use default/fallback
}
```

## Performance Tips

### 1. Cache Paths

Instead of repeated calls:
```c
// Bad: Repeated path construction
for (int i = 0; i < 100; i++) {
    char path[1024];
    aura_fs_get_data_path("file.txt", path, sizeof(path));
    // use path
}

// Good: Cache once
char path[1024];
aura_fs_get_data_path("file.txt", path, sizeof(path));
for (int i = 0; i < 100; i++) {
    // use path
}
```

### 2. Lazy Configuration Loading

```c
// Load only what you need
int log_level = aura_config_get_int("log_level", 3);  // Get when needed

// Not: Load everything at startup
for (all_possible_keys) {
    aura_config_get_*(...);  // Unnecessary
}
```

### 3. Asset Preloading

```c
// In startup sequence, after aura_launcher_init_all_systems()
void preload_critical_assets(void) {
    aura_assets_load("images/logo.png", ASSET_TYPE_IMAGE);
    aura_assets_load("fonts/main.ttf", ASSET_TYPE_FONT);
    printf("Critical assets preloaded\n");
}

// Then load other assets on-demand
if (!aura_assets_exists("images/icon.png")) {
    aura_assets_load("images/icon.png", ASSET_TYPE_IMAGE);
}
```

## Best Practices

### 1. Always Initialize

```c
int main(int argc, char *argv[]) {
    if (!gtk_init_check(&argc, &argv)) {
        return 1;
    }
    
    // Always call this FIRST after GTK init
    if (!aura_launcher_init_all_systems()) {
        return 1;
    }
    
    // Now all paths are ready
    // ... rest of application
}
```

### 2. Always Cleanup

```c
int main(int argc, char *argv[]) {
    // ... application code ...
    
    // Always call this LAST before exit
    aura_launcher_cleanup_all_systems();
    return 0;
}
```

### 3. Use Descriptive Names

```c
// Good path reference
char config_file[1024];
aura_fs_get_config_path("user_settings.cfg", config_file, sizeof(config_file));

// Less clear
char path[1024];
aura_fs_get_config_path("us.cfg", path, sizeof(path));
```

### 4. Check Return Values

```c
if (!aura_fs_mkdir_recursive(some_path)) {
    fprintf(stderr, "Failed to create directory\n");
    return false;  // Handle error
}

// Not:
aura_fs_mkdir_recursive(some_path);  // Ignore result
```

## Migration Timeline

### Phase 1: Core Integration (Day 1)
- Update main.c
- Call aura_launcher_init_all_systems()
- Test basic startup

### Phase 2: Module Updates (Day 2-3)
- Update auth.c to use proper paths
- Update db.c to use proper paths
- Update UI modules for asset paths

### Phase 3: Configuration Integration (Day 4)
- Replace hardcoded values with config_get_*
- Save user preferences with config_set_*
- Test configuration persistence

### Phase 4: Testing & Verification (Day 5)
- First run test
- Portability test (run from different directory)
- Clean system test
- Backup/restore test

## Reference: Module APIs

### Quick API Reference

```c
// Filesystem
aura_fs_init()
aura_fs_verify_structure()
aura_fs_get_data_path(name, buf, len)
aura_fs_get_asset_path(name, buf, len)
aura_fs_get_config_path(name, buf, len)
aura_fs_file_exists(path)
aura_fs_dir_exists(path)

// Assets
aura_assets_init()
aura_assets_load(name, type)
aura_assets_exists(name)
aura_assets_verify_critical()

// Configuration
aura_config_init()
aura_config_load(path)
aura_config_save(path)
aura_config_get_string(key, default)
aura_config_get_int(key, default)
aura_config_get_bool(key, default)
aura_config_set_string(key, value)
aura_config_set_int(key, value)
aura_config_set_bool(key, value)

// Launcher
aura_launcher_init_all_systems()
aura_launcher_cleanup_all_systems()
aura_launcher_verify_integrity()
aura_launcher_get_resource_path(type, name, buf, len)
```

## Support

For integration questions:
1. Check PACKAGING_SYSTEM.md for architecture details
2. Review module headers for function documentation
3. Examine existing integrations in main.c
4. Look at example code in this guide

---

**AURA Professional Packaging System**
**Developer Integration Guide v1.0**
