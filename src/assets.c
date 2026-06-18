#include "assets.h"
#include "filesystem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CACHED_ASSETS 512

typedef struct {
    char name[256];
    char path[1024];
    AuraAssetType type;
    bool loaded;
} AuraCachedAsset;

static AuraCachedAsset g_asset_cache[MAX_CACHED_ASSETS];
static int g_asset_count = 0;

/**
 * Initialize the assets module
 */
bool aura_assets_init(void) {
    printf("[AURA] Initializing assets module...\n");
    
    memset(g_asset_cache, 0, sizeof(g_asset_cache));
    g_asset_count = 0;
    
    // Verify critical asset directories exist
    char assets_path[1024];
    snprintf(assets_path, sizeof(assets_path), "%s/assets", aura_fs_get_app_root());
    
    if (!aura_fs_dir_exists(assets_path)) {
        printf("[AURA] Creating assets directory: %s\n", assets_path);
        aura_fs_mkdir_recursive(assets_path);
    }
    
    // Create subdirectories for assets
    const char *asset_dirs[] = {
        "images", "sounds", "fonts", "animations", NULL
    };
    
    for (int i = 0; asset_dirs[i] != NULL; i++) {
        char subdir[1024];
        snprintf(subdir, sizeof(subdir), "%s/%s", assets_path, asset_dirs[i]);
        if (!aura_fs_dir_exists(subdir)) {
            aura_fs_mkdir_recursive(subdir);
        }
        printf("[AURA] Asset subdirectory ready: %s\n", asset_dirs[i]);
    }
    
    printf("[AURA] Assets module initialized\n");
    return true;
}

/**
 * Get asset type from file extension
 */
AuraAssetType aura_assets_get_type_from_extension(const char *filename) {
    if (filename == NULL) {
        return ASSET_TYPE_UNKNOWN;
    }
    
    const char *ext = strrchr(filename, '.');
    if (ext == NULL) {
        return ASSET_TYPE_UNKNOWN;
    }
    
    if (strcmp(ext, ".png") == 0 || strcmp(ext, ".jpg") == 0 || 
        strcmp(ext, ".jpeg") == 0 || strcmp(ext, ".bmp") == 0 ||
        strcmp(ext, ".gif") == 0) {
        return ASSET_TYPE_IMAGE;
    }
    
    if (strcmp(ext, ".wav") == 0 || strcmp(ext, ".mp3") == 0 || 
        strcmp(ext, ".ogg") == 0 || strcmp(ext, ".flac") == 0) {
        return ASSET_TYPE_SOUND;
    }
    
    if (strcmp(ext, ".ttf") == 0 || strcmp(ext, ".otf") == 0) {
        return ASSET_TYPE_FONT;
    }
    
    if (strcmp(ext, ".cfg") == 0 || strcmp(ext, ".conf") == 0 || 
        strcmp(ext, ".json") == 0 || strcmp(ext, ".ini") == 0) {
        return ASSET_TYPE_CONFIG;
    }
    
    return ASSET_TYPE_UNKNOWN;
}

/**
 * Find or create a cache entry for an asset
 */
static AuraCachedAsset *find_or_create_asset_entry(const char *asset_name) {
    // Look for existing entry
    for (int i = 0; i < g_asset_count; i++) {
        if (strcmp(g_asset_cache[i].name, asset_name) == 0) {
            return &g_asset_cache[i];
        }
    }
    
    // Create new entry if space available
    if (g_asset_count < MAX_CACHED_ASSETS) {
        AuraCachedAsset *entry = &g_asset_cache[g_asset_count++];
        strncpy(entry->name, asset_name, sizeof(entry->name) - 1);
        return entry;
    }
    
    return NULL;
}

/**
 * Load an asset from disk
 */
bool aura_assets_load(const char *asset_name, AuraAssetType type) {
    if (asset_name == NULL) {
        return false;
    }
    
    AuraCachedAsset *entry = find_or_create_asset_entry(asset_name);
    if (entry == NULL) {
        fprintf(stderr, "[AURA ERROR] Asset cache full, cannot load: %s\n", asset_name);
        return false;
    }
    
    // Build full asset path
    char asset_path[1024];
    if (!aura_fs_get_asset_path(asset_name, asset_path, sizeof(asset_path))) {
        fprintf(stderr, "[AURA ERROR] Failed to build asset path for: %s\n", asset_name);
        return false;
    }
    
    // Check if file exists
    if (!aura_fs_file_exists(asset_path)) {
        fprintf(stderr, "[AURA WARNING] Asset not found: %s\n", asset_path);
        return false;
    }
    
    // Mark as loaded in cache
    strncpy(entry->path, asset_path, sizeof(entry->path) - 1);
    entry->type = type;
    entry->loaded = true;
    
    printf("[AURA] Loaded asset: %s\n", asset_name);
    return true;
}

/**
 * Get the full path to an asset
 */
bool aura_assets_get_path(const char *asset_name, char *out_path, size_t max_len) {
    if (asset_name == NULL || out_path == NULL) {
        return false;
    }
    
    return aura_fs_get_asset_path(asset_name, out_path, max_len);
}

/**
 * Check if an asset exists
 */
bool aura_assets_exists(const char *asset_name) {
    if (asset_name == NULL) {
        return false;
    }
    
    char asset_path[1024];
    if (!aura_fs_get_asset_path(asset_name, asset_path, sizeof(asset_path))) {
        return false;
    }
    
    return aura_fs_file_exists(asset_path);
}

/**
 * Verify all critical assets are present
 */
bool aura_assets_verify_critical(void) {
    printf("[AURA] Verifying critical assets...\n");
    
    // Define critical assets required for the application
    const char *critical_assets[] = {
        // Add critical assets here as your project grows
        // Example: "images/logo.png", "fonts/main.ttf"
        NULL  // Null terminator
    };
    
    // For now, just verify the asset directories exist
    const char *required_dirs[] = {
        "images", "sounds", "fonts", "animations", NULL
    };
    
    for (int i = 0; required_dirs[i] != NULL; i++) {
        char dir_path[1024];
        if (!aura_fs_get_asset_path(required_dirs[i], dir_path, sizeof(dir_path))) {
            fprintf(stderr, "[AURA ERROR] Failed to verify asset directory: %s\n", required_dirs[i]);
            return false;
        }
        
        if (!aura_fs_dir_exists(dir_path)) {
            fprintf(stderr, "[AURA ERROR] Critical asset directory missing: %s\n", dir_path);
            return false;
        }
    }
    
    printf("[AURA] All critical assets verified\n");
    return true;
}

/**
 * Cleanup assets module
 */
void aura_assets_cleanup(void) {
    printf("[AURA] Cleaning up assets...\n");
    
    // Free any loaded resources
    for (int i = 0; i < g_asset_count; i++) {
        if (g_asset_cache[i].loaded) {
            // Additional cleanup for specific asset types could go here
            g_asset_cache[i].loaded = false;
        }
    }
    
    g_asset_count = 0;
}
