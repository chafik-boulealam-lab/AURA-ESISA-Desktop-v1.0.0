#ifndef AURA_CONFIG_H
#define AURA_CONFIG_H

#include <stdbool.h>

/**
 * AURA Configuration Module
 * Manages application settings and configuration files
 */

typedef struct {
    char app_name[128];
    char app_version[32];
    int window_width;
    int window_height;
    bool vsync_enabled;
    int log_level;  // 0=silent, 1=error, 2=warning, 3=info, 4=debug
    bool start_fullscreen;
    char theme[64];
    bool auto_save_enabled;
    int auto_save_interval;  // in seconds
    char groq_api_key[256];
} AuraConfig;

extern AuraConfig g_aura_config;

/**
 * Initialize configuration module
 * Loads config file if exists, otherwise creates default config
 * @return true if initialization successful
 */
bool aura_config_init(void);

/**
 * Load configuration from file
 * @param config_path Path to configuration file
 * @return true if config loaded successfully
 */
bool aura_config_load(const char *config_path);

/**
 * Save configuration to file
 * @param config_path Path where config should be saved
 * @return true if config saved successfully
 */
bool aura_config_save(const char *config_path);

/**
 * Set a configuration value
 * @param key Configuration key
 * @param value Configuration value
 * @return true if value set successfully
 */
bool aura_config_set_string(const char *key, const char *value);

/**
 * Set an integer configuration value
 * @param key Configuration key
 * @param value Integer value
 * @return true if value set successfully
 */
bool aura_config_set_int(const char *key, int value);

/**
 * Set a boolean configuration value
 * @param key Configuration key
 * @param value Boolean value
 * @return true if value set successfully
 */
bool aura_config_set_bool(const char *key, bool value);

/**
 * Get a string configuration value
 * @param key Configuration key
 * @param default_value Default value if key not found
 * @return Configuration value or default
 */
const char *aura_config_get_string(const char *key, const char *default_value);

/**
 * Get an integer configuration value
 * @param key Configuration key
 * @param default_value Default value if key not found
 * @return Configuration value or default
 */
int aura_config_get_int(const char *key, int default_value);

/**
 * Get a boolean configuration value
 * @param key Configuration key
 * @param default_value Default value if key not found
 * @return Configuration value or default
 */
bool aura_config_get_bool(const char *key, bool default_value);

/**
 * Reset configuration to defaults
 */
void aura_config_reset_to_defaults(void);

/**
 * Cleanup configuration module
 */
void aura_config_cleanup(void);

#endif // AURA_CONFIG_H
