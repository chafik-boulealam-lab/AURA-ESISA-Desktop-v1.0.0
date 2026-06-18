#include "filesystem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#define PATH_SEPARATOR "\\"
#define mkdir(path, mode) _mkdir(path)
#else
#include <unistd.h>
#define PATH_SEPARATOR "/"
#endif

AuraFilesystem g_aura_fs = {0};

/**
 * Get the directory containing the executable
 * Used to establish application root
 */
static char *get_exe_directory(void) {
    static char exe_dir[1024] = {0};
    
    if (exe_dir[0] != '\0') {
        return exe_dir;  // Already computed
    }

#ifdef _WIN32
    char exe_path[1024];
    if (GetModuleFileNameA(NULL, exe_path, sizeof(exe_path)) == 0) {
        strcpy(exe_dir, ".");
        return exe_dir;
    }
    
    // Find the last backslash
    char *last_slash = strrchr(exe_path, '\\');
    if (last_slash != NULL) {
        *last_slash = '\0';
        strcpy(exe_dir, exe_path);
    } else {
        strcpy(exe_dir, ".");
    }
#else
    // On Unix-like systems, try to read /proc/self/exe
    ssize_t len = readlink("/proc/self/exe", exe_dir, sizeof(exe_dir) - 1);
    if (len != -1) {
        exe_dir[len] = '\0';
        char *last_slash = strrchr(exe_dir, '/');
        if (last_slash != NULL) {
            *last_slash = '\0';
        }
    } else {
        strcpy(exe_dir, ".");
    }
#endif
    
    return exe_dir;
}

/**
 * Initialize the filesystem module
 */
bool aura_fs_init(void) {
    const char *exe_dir = get_exe_directory();
    
    // Set application root
    snprintf(g_aura_fs.app_root, sizeof(g_aura_fs.app_root), "%s", exe_dir);
    
    // Build all directory paths
    snprintf(g_aura_fs.data_dir, sizeof(g_aura_fs.data_dir), "%s%sdata", 
             g_aura_fs.app_root, PATH_SEPARATOR);
    snprintf(g_aura_fs.assets_dir, sizeof(g_aura_fs.assets_dir), "%s%sassets", 
             g_aura_fs.app_root, PATH_SEPARATOR);
    snprintf(g_aura_fs.config_dir, sizeof(g_aura_fs.config_dir), "%s%sconfig", 
             g_aura_fs.app_root, PATH_SEPARATOR);
    snprintf(g_aura_fs.cache_dir, sizeof(g_aura_fs.cache_dir), "%s%scache", 
             g_aura_fs.app_root, PATH_SEPARATOR);
    
    // Build file paths
    snprintf(g_aura_fs.accounts_file, sizeof(g_aura_fs.accounts_file), "%s%saccounts.txt", 
             g_aura_fs.data_dir, PATH_SEPARATOR);
    snprintf(g_aura_fs.settings_file, sizeof(g_aura_fs.settings_file), "%s%saura.cfg", 
             g_aura_fs.config_dir, PATH_SEPARATOR);
    
    printf("[AURA] Filesystem initialized\n");
    printf("[AURA] App root: %s\n", g_aura_fs.app_root);
    
    return aura_fs_verify_structure();
}

/**
 * Verify that all required directories exist and create if needed
 */
bool aura_fs_verify_structure(void) {
    const char *dirs[] = {
        g_aura_fs.data_dir,
        g_aura_fs.assets_dir,
        g_aura_fs.config_dir,
        g_aura_fs.cache_dir,
        NULL
    };
    
    for (int i = 0; dirs[i] != NULL; i++) {
        if (!aura_fs_mkdir_recursive(dirs[i])) {
            fprintf(stderr, "[AURA ERROR] Failed to create directory: %s\n", dirs[i]);
            return false;
        }
        printf("[AURA] Directory verified: %s\n", dirs[i]);
    }

    char reports_dir[1024];
    snprintf(reports_dir, sizeof(reports_dir), "%s%sreports", g_aura_fs.data_dir, PATH_SEPARATOR);
    if (!aura_fs_mkdir_recursive(reports_dir)) {
        fprintf(stderr, "[AURA ERROR] Failed to create directory: %s\n", reports_dir);
        return false;
    }
    printf("[AURA] Directory verified: %s\n", reports_dir);
    
    return true;
}

/**
 * Get the full path to an asset file
 */
bool aura_fs_get_asset_path(const char *resource_name, char *out_path, size_t max_len) {
    if (resource_name == NULL || out_path == NULL) {
        return false;
    }
    
    snprintf(out_path, max_len, "%s%s%s", g_aura_fs.assets_dir, PATH_SEPARATOR, resource_name);
    return true;
}

/**
 * Get the full path to a data file
 */
bool aura_fs_get_data_path(const char *filename, char *out_path, size_t max_len) {
    if (filename == NULL || out_path == NULL) {
        return false;
    }
    
    snprintf(out_path, max_len, "%s%s%s", g_aura_fs.data_dir, PATH_SEPARATOR, filename);
    return true;
}

/**
 * Get the full path to a config file
 */
bool aura_fs_get_config_path(const char *filename, char *out_path, size_t max_len) {
    if (filename == NULL || out_path == NULL) {
        return false;
    }
    
    snprintf(out_path, max_len, "%s%s%s", g_aura_fs.config_dir, PATH_SEPARATOR, filename);
    return true;
}

/**
 * Get the full path to a cache file
 */
bool aura_fs_get_cache_path(const char *filename, char *out_path, size_t max_len) {
    if (filename == NULL || out_path == NULL) {
        return false;
    }
    
    snprintf(out_path, max_len, "%s%s%s", g_aura_fs.cache_dir, PATH_SEPARATOR, filename);
    return true;
}

/**
 * Check if a file exists
 */
bool aura_fs_file_exists(const char *filepath) {
    if (filepath == NULL) {
        return false;
    }
    
    struct stat buffer;
    return (stat(filepath, &buffer) == 0);
}

/**
 * Check if a directory exists
 */
bool aura_fs_dir_exists(const char *dirpath) {
    if (dirpath == NULL) {
        return false;
    }
    
    struct stat buffer;
    return (stat(dirpath, &buffer) == 0) && (buffer.st_mode & S_IFDIR);
}

/**
 * Create a directory recursively
 */
bool aura_fs_mkdir_recursive(const char *dirpath) {
    if (dirpath == NULL || dirpath[0] == '\0') {
        return false;
    }
    
    // If directory already exists, consider it success
    if (aura_fs_dir_exists(dirpath)) {
        return true;
    }
    
    // Make a copy to work with
    char temp_path[1024];
    strncpy(temp_path, dirpath, sizeof(temp_path) - 1);
    temp_path[sizeof(temp_path) - 1] = '\0';

    char *iter_start = temp_path + 1;
#ifdef _WIN32
    // Skip drive prefix (e.g. "C:\") so we don't attempt mkdir("C:").
    if (((temp_path[0] >= 'A' && temp_path[0] <= 'Z') ||
         (temp_path[0] >= 'a' && temp_path[0] <= 'z')) &&
        temp_path[1] == ':') {
        iter_start = temp_path + 3;
    }
#endif
    
    // Create parent directories
    for (char *p = iter_start; *p != '\0'; p++) {
#ifdef _WIN32
        if (*p == '\\' || *p == '/') {
#else
        if (*p == '/') {
#endif
            *p = '\0';
            if (!aura_fs_dir_exists(temp_path)) {
                if (mkdir(temp_path, 0755) != 0) {
                    return false;
                }
            }
            *p = PATH_SEPARATOR[0];
        }
    }
    
    // Create final directory
    if (mkdir(dirpath, 0755) != 0) {
        // It's ok if it already exists
        return aura_fs_dir_exists(dirpath);
    }
    
    return true;
}

/**
 * Get the application root directory
 */
const char *aura_fs_get_app_root(void) {
    return g_aura_fs.app_root;
}

/**
 * Cleanup filesystem module
 */
void aura_fs_cleanup(void) {
    // Currently no resources to free
}
