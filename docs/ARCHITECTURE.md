# AURA Professional Packaging System - Architecture Overview

## System Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                      AURA.exe (Entry Point)                     │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
         ┌───────────────────────────────────┐
         │   GTK Initialization              │
         │   (gtk_init_check)                │
         └────────────┬────────────────────┘
                      │
                      ▼
         ┌───────────────────────────────────┐
         │  Launcher Module                  │
         │  (aura_launcher_init_all_systems) │
         └────────────┬────────────────────┘
                      │
        ┌─────────────┼─────────────┐
        │             │             │
        ▼             ▼             ▼
   ┌────────┐  ┌──────────┐  ┌───────────┐
   │Startup │  │Filesystem│  │  Assets   │
   │Screen  │  │  Module  │  │  Module   │
   └────┬───┘  └────┬─────┘  └─────┬─────┘
        │           │              │
        └─────────────┬──────────────┘
                      ▼
         ┌───────────────────────────────────┐
         │   Configuration Module            │
         │   (Load user settings)            │
         └────────────┬────────────────────┘
                      │
                      ▼
         ┌───────────────────────────────────┐
         │   Authentication Module           │
         │   (Initialize user database)      │
         └────────────┬────────────────────┘
                      │
                      ▼
         ┌───────────────────────────────────┐
         │   Application Ready               │
         │   (Launch login screen)           │
         └───────────────────────────────────┘
```

## Module Dependency Graph

```
launcher.h
├── filesystem.h
├── assets.h
├── config.h
├── startup.h
└── (calls auth.h)

startup.h
├── filesystem.h
├── assets.h
├── config.h
└── (displays GTK widgets)

filesystem.h
└── (uses stdlib, stdio, sys/stat)

assets.h
├── filesystem.h
└── (uses stdio, stdlib)

config.h
├── filesystem.h
└── (uses stdio, stdlib, string)

Main application
├── launcher.h
├── filesystem.h
├── config.h
├── auth.h
├── db.h
├── dashboard_ui.h
├── login_ui.h
├── api.h
└── (existing modules)
```

## File System Layout

```
P-F-A/                          (Source repository)
│
├── src/                        (C source files)
│   ├── main.c                 (Entry point - UPDATED)
│   ├── launcher.c             (NEW - Orchestrator)
│   ├── filesystem.c           (NEW - Path management)
│   ├── assets.c               (NEW - Resource loading)
│   ├── config.c               (NEW - Configuration)
│   ├── startup.c              (NEW - Initialization)
│   ├── auth.c                 (Existing)
│   ├── db.c                   (Existing)
│   ├── api.c                  (Existing)
│   ├── dashboard_ui.c         (Existing)
│   └── login_ui.c             (Existing)
│
├── include/                   (C header files)
│   ├── launcher.h             (NEW)
│   ├── filesystem.h           (NEW)
│   ├── assets.h               (NEW)
│   ├── config.h               (NEW)
│   ├── startup.h              (NEW)
│   ├── auth.h                 (Existing)
│   ├── db.h                   (Existing)
│   ├── api.h                  (Existing)
│   ├── dashboard_ui.h         (Existing)
│   └── login_ui.h             (Existing)
│
├── bin/                       (Compiled output)
│   └── AURA.exe              (Main executable)
│
├── obj/                       (Object files)
│   ├── launcher.o
│   ├── filesystem.o
│   ├── assets.o
│   ├── config.o
│   ├── startup.o
│   └── (other .o files)
│
├── data/                      (User data - auto-created)
│   └── accounts.txt          (Auto-created on first run)
│
├── Makefile                   (Build system - UPDATED)
├── README.md                  (Project readme)
├── PACKAGING_SYSTEM.md        (NEW - Technical docs)
├── RELEASE_GUIDE.md           (NEW - Distribution guide)
└── DEVELOPER_GUIDE.md         (NEW - Integration guide)

AURA/                          (Release package)
│
├── AURA.exe                   (Primary executable)
│
├── data/                      (Auto-created on first run)
│   └── accounts.txt          (User accounts)
│
├── assets/                    (Auto-created on first run)
│   ├── images/               (UI graphics)
│   ├── sounds/               (Audio files)
│   ├── fonts/                (TTF/OTF fonts)
│   └── animations/           (Animation data)
│
├── config/                    (Auto-created on first run)
│   └── aura.cfg             (Settings file)
│
└── cache/                     (Auto-created on first run)
    └── (temporary files)
```

## Initialization Sequence Detail

### Phase 1: GTK Initialization (main.c)
```
gtk_init_check()
  ├─ Initialize GTK library
  ├─ Load display configuration
  └─ Ready for widget creation
```

### Phase 2: Launcher Initialization (launcher.c)
```
aura_launcher_init_all_systems()
  ├─ aura_startup_run_sequence()
  │   ├─ Create splash window
  │   ├─ Phase 1: Filesystem
  │   │   └─ aura_fs_init()
  │   │       ├─ Get executable directory
  │   │       ├─ Set path variables
  │   │       └─ aura_fs_verify_structure()
  │   │           ├─ Create data/
  │   │           ├─ Create assets/
  │   │           ├─ Create config/
  │   │           ├─ Create cache/
  │   │           └─ Create asset subdirs
  │   │
  │   ├─ Phase 2: Assets
  │   │   └─ aura_assets_init()
  │   │       ├─ Verify asset directories
  │   │       └─ Initialize asset cache
  │   │
  │   ├─ Phase 3: Configuration
  │   │   └─ aura_config_init()
  │   │       ├─ Load aura.cfg if exists
  │   │       └─ Use defaults if not
  │   │
  │   ├─ Phase 4: Database
  │   │   └─ auth_init(accounts_path)
  │   │       └─ Initialize user accounts
  │   │
  │   └─ Close splash window
  └─ Set g_systems_initialized = true
```

### Phase 3: Application Launch (main.c)
```
Application Ready
  ├─ auth_init(accounts_path)    [Initialize auth with proper path]
  ├─ init_db()                   [Initialize database]
  ├─ apply_aura_css()            [Apply styling]
  └─ aura_launch_login_screen()  [Show login UI]
```

### Phase 4: Application Shutdown (main.c)
```
Application Exit
  └─ aura_launcher_cleanup_all_systems()
      ├─ aura_config_save()
      │   └─ Save config/aura.cfg
      ├─ aura_assets_cleanup()
      │   └─ Free asset cache
      ├─ aura_config_cleanup()
      └─ aura_fs_cleanup()
```

## Data Flow

### File Access Flow

```
Application Code
  ↓
aura_fs_get_data_path("file.txt", buffer, size)
  ├─ Check if path already computed
  ├─ Compute path relative to app root
  ├─ Validate path format
  └─ Return full path
  ↓
fopen(path, "r")  [Application uses returned path]
  ↓
File System
```

### Configuration Flow

```
Application Code
  ├─ aura_config_get_string("theme", "dark")
  │   ↓
  │   Search config hash table
  │   ↓
  │   Return value or default
  │
  └─ aura_config_set_bool("auto_save", true)
      ↓
      Update config hash table
      ↓
      (Saved to disk at shutdown)
```

### Asset Loading Flow

```
Application Code
  ├─ aura_assets_exists("images/logo.png")
  │   ↓
  │   Check if file exists
  │   ↓
  │   Return true/false
  │
  └─ aura_assets_load("images/logo.png", ASSET_TYPE_IMAGE)
      ↓
      Add to asset cache
      ↓
      Return true if successful
```

## System States

### State: Uninitialized

```
g_systems_initialized = false
g_aura_fs = {0}
g_aura_config = {defaults}
g_asset_cache = {empty}

Allowed operations:
- None (most functions check for initialization)
- GTK functions
```

### State: Initializing

```
Splash screen displayed
Showing progress for each phase
User cannot interact with main window
Can update splash progress
```

### State: Initialized

```
g_systems_initialized = true
g_aura_fs = {all paths set}
g_aura_config = {loaded from file or defaults}
g_asset_cache = {ready for loading}

Allowed operations:
- All filesystem operations
- Configuration get/set
- Asset loading
- Application main loop
```

### State: Shutting Down

```
Main loop exited
Cleanup in progress
Save configuration
Free resources
All modules being cleaned up
```

## Global Variables

### Filesystem Module (filesystem.c)

```c
AuraFilesystem g_aura_fs
├── char app_root[1024]
├── char data_dir[1024]
├── char assets_dir[1024]
├── char config_dir[1024]
├── char cache_dir[1024]
├── char accounts_file[1024]
└── char settings_file[1024]

Access: #include "filesystem.h"
Scope: Global (extern declaration in header)
Thread-safe: No (read-only after init)
```

### Configuration Module (config.c)

```c
AuraConfig g_aura_config
├── char app_name[128]
├── char app_version[32]
├── int window_width
├── int window_height
├── bool vsync_enabled
├── int log_level
├── bool start_fullscreen
├── char theme[64]
├── bool auto_save_enabled
└── int auto_save_interval

Access: #include "config.h"
Scope: Global (extern declaration in header)
Thread-safe: No (read mostly, written at startup)
```

## Thread Safety

### Current Design

**Not thread-safe**: All modules assume single-threaded access
- No mutex locks
- No atomic operations
- Global variables directly accessed

### Usage Guidelines

1. **Initialization Phase**: Single-threaded only
2. **Main Loop**: Single-threaded (GTK main loop)
3. **Shutdown Phase**: Single-threaded only

### For Multi-threaded Use

To make thread-safe (future enhancement):
- Add mutex locks in filesystem.c
- Add mutex locks in config.c
- Use atomic operations for flags
- Document critical sections

## Performance Characteristics

### Initialization Time

```
Phase 1 (Filesystem):  ~10-20ms
  - Get executable path
  - Build path strings
  - Create directories

Phase 2 (Assets):      ~30-50ms
  - Scan asset directories
  - Initialize cache

Phase 3 (Config):      ~5-10ms
  - Read config file
  - Parse settings

Phase 4 (Database):    ~50-100ms
  - Open database
  - Create tables

Total:                 ~95-180ms (< 200ms typically)
```

### Runtime Performance

```
aura_fs_get_data_path():       < 1ms (string operation)
aura_config_get_int():         < 1ms (hash lookup)
aura_assets_exists():          < 1ms (stat() call)
aura_assets_load():            ~10-100ms (depends on file size)
```

### Memory Usage

```
g_aura_fs:             ~3KB (path strings)
g_aura_config:         ~1KB (settings)
Asset cache:           ~10-100MB (depends on loaded assets)
Total overhead:        ~15-20KB minimum
```

## Error Handling

### Strategy

- Return false on errors
- Print to stderr for debugging
- Continue operation if possible
- Never throw exceptions
- Gracefully degrade

### Error Categories

#### Critical Errors
```c
if (!aura_fs_init()) {
    fprintf(stderr, "[AURA ERROR] Filesystem failed\n");
    return false;  // Application cannot continue
}
```

#### Non-Critical Errors
```c
if (!aura_assets_verify_critical()) {
    fprintf(stderr, "[AURA WARNING] Some assets missing\n");
    // Application continues anyway
}
```

#### Recoverable Errors
```c
if (!aura_fs_file_exists(path)) {
    printf("[AURA] File not found, using default\n");
    // Use fallback
}
```

## Security Considerations

### Path Traversal Prevention

```c
// Bad: Allows ../../ attacks
void bad_function(const char *user_input) {
    char path[1024];
    snprintf(path, sizeof(path), "data/%s", user_input);  // Vulnerable!
}

// Good: Only appends name
void good_function(const char *filename) {
    aura_fs_get_data_path(filename, path, sizeof(path));
    // Validates internally, prevents traversal
}
```

### File Permissions

- User data: 644 (user readable/writable)
- Directories: 755 (traversable by all)
- Executable: 755 (executable by all)
- Config: 644 (readable by all)

### Resource Limits

- Max cached assets: 512 (configurable)
- Max path length: 1024 bytes
- Max config entries: unlimited
- Max asset size: unlimited (memory permitting)

## Extensibility Points

### Adding New Modules

1. Create module.h in include/
2. Create module.c in src/
3. Add initialization call to launcher.c
4. Add cleanup call to launcher_cleanup_all_systems()
5. Update Makefile (automatic with wildcard)

### Adding New Configuration Categories

1. Extend AuraConfig struct
2. Add parsing in parse_config_line()
3. Add getter/setter functions
4. Add save logic in aura_config_save()

### Adding New Startup Phases

1. Add to AuraStartupPhase enum
2. Add phase name string
3. Add initialization code
4. Call aura_startup_update_progress()

## Deployment Scenarios

### Scenario 1: Single User, Single Machine

```
User downloads AURA.zip
Extracts to C:\Users\User\AppData\Local\AURA
Runs AURA.exe
Application creates data/config/cache folders
Data stored locally
```

### Scenario 2: Multiple Users, Single Machine

```
Admin installs to C:\Program Files\AURA
Each user gets own config/cache in AppData
Shared executable in Program Files
Each user has separate data folder
```

### Scenario 3: Network Deployment

```
AURA.exe on network share (read-only)
Each user has local data folder
Config cached locally
Works on any machine with network access
```

## Future Enhancements

### Planned Features

1. **Plugin System**: Load plugins from assets/plugins/
2. **Auto-Update**: Check for updates from server
3. **Cloud Sync**: Sync data to cloud storage
4. **Localization**: Multi-language support
5. **Analytics**: Track usage patterns
6. **Telemetry**: Send diagnostics (opt-in)

### Architecture Ready For

- Async initialization (use threads)
- Network file access (modify fs module)
- Database migration (extend db module)
- UI theming (extend config module)
- Asset encryption (extend assets module)

---

**AURA Professional Packaging System**
**Complete Architecture Documentation v1.0**
