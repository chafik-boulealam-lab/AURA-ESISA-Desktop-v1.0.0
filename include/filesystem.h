#ifndef AURA_FILESYSTEM_H
#define AURA_FILESYSTEM_H

#include <stdbool.h>
#include <stddef.h>

/**
 * AURA Filesystem Module
 * Manages application directory structure and resource paths
 * Ensures portable application layout across different systems
 */

typedef struct {
    char app_root[1024];        // Application root directory
    char data_dir[1024];        // User data directory
    char assets_dir[1024];      // Assets directory (images, sounds, etc.)
    char config_dir[1024];      // Configuration directory
    char cache_dir[1024];       // Temporary cache directory
    char accounts_file[1024];   // Path to accounts.txt
    char settings_file[1024];   // Path to settings.cfg
} AuraFilesystem;

extern AuraFilesystem g_aura_fs;

/**
 * Initialize the filesystem module
 * Creates all necessary directories and initializes path structure
 * @return true if initialization successful, false otherwise
 */
bool aura_fs_init(void);

/**
 * Verify that all required directories exist
 * Creates missing directories as needed
 * @return true if all directories valid/created, false on error
 */
bool aura_fs_verify_structure(void);

/**
 * Get the full path to a resource file
 * @param resource_name Name of the resource (relative path)
 * @param out_path Output buffer for full path
 * @param max_len Maximum length of output buffer
 * @return true if path successfully constructed
 */
bool aura_fs_get_asset_path(const char *resource_name, char *out_path, size_t max_len);

/**
 * Get the full path to a data file
 * @param filename Name of the data file
 * @param out_path Output buffer for full path
 * @param max_len Maximum length of output buffer
 * @return true if path successfully constructed
 */
bool aura_fs_get_data_path(const char *filename, char *out_path, size_t max_len);

/**
 * Get the full path to a config file
 * @param filename Name of the config file
 * @param out_path Output buffer for full path
 * @param max_len Maximum length of output buffer
 * @return true if path successfully constructed
 */
bool aura_fs_get_config_path(const char *filename, char *out_path, size_t max_len);

/**
 * Get the full path to a cache file
 * @param filename Name of the cache file
 * @param out_path Output buffer for full path
 * @param max_len Maximum length of output buffer
 * @return true if path successfully constructed
 */
bool aura_fs_get_cache_path(const char *filename, char *out_path, size_t max_len);

/**
 * Check if a file exists at the given path
 * @param filepath Full path to file
 * @return true if file exists, false otherwise
 */
bool aura_fs_file_exists(const char *filepath);

/**
 * Check if a directory exists at the given path
 * @param dirpath Full path to directory
 * @return true if directory exists, false otherwise
 */
bool aura_fs_dir_exists(const char *dirpath);

/**
 * Create a directory (with parent directories if needed)
 * @param dirpath Full path to directory
 * @return true if directory created or already exists, false on error
 */
bool aura_fs_mkdir_recursive(const char *dirpath);

/**
 * Get the application root directory
 * @return Pointer to app root path string
 */
const char *aura_fs_get_app_root(void);

/**
 * Cleanup filesystem module
 */
void aura_fs_cleanup(void);

#endif // AURA_FILESYSTEM_H
