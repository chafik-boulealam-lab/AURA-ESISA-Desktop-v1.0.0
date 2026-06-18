#include "launcher.h"
#include "filesystem.h"
#include "assets.h"
#include "config.h"
#include "startup.h"
#include "auth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AURA_APP_NAME "AURA Simulator"
#define AURA_VERSION "1.0.0"

static bool g_systems_initialized = false;

/**
 * Initialize all AURA subsystems
 */
bool aura_launcher_init_all_systems(void) {
    if (g_systems_initialized) {
        printf("[AURA] Systems already initialized\n");
        return true;
    }
    
    printf("\n");
    /* Use ASCII-friendly banner on Windows consoles to avoid Unicode rendering issues */
    printf("+------------------------------------------------------------+\n");
    printf("|  AURA SIMULATOR v%s                                     |\n", AURA_VERSION);
    printf("|  Professional Application Launcher                        |\n");
    printf("+------------------------------------------------------------+\n\n");
    
    // Run the full startup sequence with visual feedback
    if (!aura_startup_run_sequence()) {
        fprintf(stderr, "[AURA ERROR] Startup sequence failed\n");
        return false;
    }
    
    g_systems_initialized = true;
    return true;
}

/**
 * Cleanup all AURA subsystems
 */
void aura_launcher_cleanup_all_systems(void) {
    if (!g_systems_initialized) {
        return;
    }
    
    printf("[AURA] Shutting down all systems...\n");
    
    // Save configuration
    char config_path[1024];
    if (aura_fs_get_config_path("aura.cfg", config_path, sizeof(config_path))) {
        aura_config_save(config_path);
        printf("[AURA] Configuration saved\n");
    }
    
    // Cleanup modules in reverse order
    aura_config_cleanup();
    aura_assets_cleanup();
    aura_fs_cleanup();
    
    printf("[AURA] All systems shut down\n");
    g_systems_initialized = false;
}

/**
 * Get the full path to a resource
 */
bool aura_launcher_get_resource_path(const char *resource_type,
                                      const char *resource_name,
                                      char *out_path,
                                      size_t max_len) {
    if (resource_type == NULL || resource_name == NULL || out_path == NULL) {
        return false;
    }
    
    if (strcmp(resource_type, "data") == 0) {
        return aura_fs_get_data_path(resource_name, out_path, max_len);
    } else if (strcmp(resource_type, "asset") == 0) {
        return aura_fs_get_asset_path(resource_name, out_path, max_len);
    } else if (strcmp(resource_type, "config") == 0) {
        return aura_fs_get_config_path(resource_name, out_path, max_len);
    } else if (strcmp(resource_type, "cache") == 0) {
        return aura_fs_get_cache_path(resource_name, out_path, max_len);
    }
    
    return false;
}

/**
 * Verify application integrity
 */
bool aura_launcher_verify_integrity(void) {
    printf("[AURA] Verifying application integrity...\n");
    
    // Check filesystem structure
    if (!aura_fs_verify_structure()) {
        fprintf(stderr, "[AURA ERROR] Filesystem verification failed\n");
        return false;
    }
    
    // Check critical assets
    if (!aura_assets_verify_critical()) {
        fprintf(stderr, "[AURA WARNING] Asset verification detected missing files (continuing)\n");
    }
    
    // Check configuration
    if (aura_config_get_int("log_level", -1) == -1) {
        fprintf(stderr, "[AURA ERROR] Configuration verification failed\n");
        return false;
    }
    
    printf("[AURA] ✓ Application integrity verified\n");
    return true;
}

/**
 * Get version information
 */
const char *aura_launcher_get_version(void) {
    return AURA_VERSION;
}

/**
 * Get application name
 */
const char *aura_launcher_get_app_name(void) {
    return AURA_APP_NAME;
}
