#include "config.h"
#include "filesystem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

AuraConfig g_aura_config = {0};

static const AuraConfig DEFAULT_CONFIG = {
    .app_name = "AURA Simulator",
    .app_version = "1.0.0",
    .window_width = 1280,
    .window_height = 800,
    .vsync_enabled = true,
    .log_level = 3,  // INFO
    .start_fullscreen = false,
    .theme = "dark",
    .auto_save_enabled = true,
    .auto_save_interval = 300,  // 5 minutes
    .groq_api_key = ""
};

/**
 * Initialize configuration module
 */
bool aura_config_init(void) {
    printf("[AURA] Initializing configuration module...\n");
    
    // Start with defaults
    memcpy(&g_aura_config, &DEFAULT_CONFIG, sizeof(AuraConfig));
    
    // Try to load user config
    char config_path[1024];
    if (aura_fs_get_config_path("aura.cfg", config_path, sizeof(config_path))) {
        if (aura_fs_file_exists(config_path)) {
            printf("[AURA] Loading user configuration from: %s\n", config_path);
            aura_config_load(config_path);
        } else {
            printf("[AURA] No existing config found, creating default\n");
            aura_config_save(config_path);
        }
    }
    
    printf("[AURA] Configuration loaded:\n");
    printf("       App: %s v%s\n", g_aura_config.app_name, g_aura_config.app_version);
    printf("       Window: %dx%d\n", g_aura_config.window_width, g_aura_config.window_height);
    printf("       Theme: %s\n", g_aura_config.theme);
    
    return true;
}

/**
 * Parse a configuration line
 */
static bool parse_config_line(const char *line) {
    if (line == NULL || line[0] == '\0' || line[0] == '#' || line[0] == ';') {
        return true;  // Skip empty lines and comments
    }
    
    // Find the = separator
    const char *sep = strchr(line, '=');
    if (sep == NULL) {
        return false;
    }
    
    // Extract key and value
    char key[256];
    char value[256];
    
    size_t key_len = sep - line;
    if (key_len >= sizeof(key)) {
        return false;
    }
    
    strncpy(key, line, key_len);
    key[key_len] = '\0';
    
    // Trim trailing whitespace from key
    while (key_len > 0 && (key[key_len - 1] == ' ' || key[key_len - 1] == '\t')) {
        key[--key_len] = '\0';
    }
    
    strncpy(value, sep + 1, sizeof(value) - 1);
    value[sizeof(value) - 1] = '\0';
    
    // Trim leading whitespace from value
    char *value_start = value;
    while (*value_start == ' ' || *value_start == '\t') {
        value_start++;
    }
    
    // Trim trailing whitespace from value
    size_t value_len = strlen(value_start);
    while (value_len > 0 && (value_start[value_len - 1] == ' ' || 
                             value_start[value_len - 1] == '\t' ||
                             value_start[value_len - 1] == '\n' ||
                             value_start[value_len - 1] == '\r')) {
        value_start[--value_len] = '\0';
    }
    
    // Parse known configuration values
    if (strcmp(key, "window_width") == 0) {
        g_aura_config.window_width = atoi(value_start);
    } else if (strcmp(key, "window_height") == 0) {
        g_aura_config.window_height = atoi(value_start);
    } else if (strcmp(key, "vsync_enabled") == 0) {
        g_aura_config.vsync_enabled = (strcmp(value_start, "true") == 0 || strcmp(value_start, "1") == 0);
    } else if (strcmp(key, "log_level") == 0) {
        g_aura_config.log_level = atoi(value_start);
    } else if (strcmp(key, "start_fullscreen") == 0) {
        g_aura_config.start_fullscreen = (strcmp(value_start, "true") == 0 || strcmp(value_start, "1") == 0);
    } else if (strcmp(key, "theme") == 0) {
        strncpy(g_aura_config.theme, value_start, sizeof(g_aura_config.theme) - 1);
    } else if (strcmp(key, "auto_save_enabled") == 0) {
        g_aura_config.auto_save_enabled = (strcmp(value_start, "true") == 0 || strcmp(value_start, "1") == 0);
    } else if (strcmp(key, "auto_save_interval") == 0) {
        g_aura_config.auto_save_interval = atoi(value_start);
    } else if (strcmp(key, "groq_api_key") == 0) {
        strncpy(g_aura_config.groq_api_key, value_start, sizeof(g_aura_config.groq_api_key) - 1);
        g_aura_config.groq_api_key[sizeof(g_aura_config.groq_api_key) - 1] = '\0';
    }
    
    return true;
}

/**
 * Load configuration from file
 */
bool aura_config_load(const char *config_path) {
    if (config_path == NULL) {
        return false;
    }
    
    FILE *file = fopen(config_path, "r");
    if (file == NULL) {
        fprintf(stderr, "[AURA ERROR] Failed to open config file: %s\n", config_path);
        return false;
    }
    
    char line[512];
    while (fgets(line, sizeof(line), file) != NULL) {
        parse_config_line(line);
    }
    
    fclose(file);
    return true;
}

/**
 * Save configuration to file
 */
bool aura_config_save(const char *config_path) {
    if (config_path == NULL) {
        return false;
    }
    
    FILE *file = fopen(config_path, "w");
    if (file == NULL) {
        fprintf(stderr, "[AURA ERROR] Failed to open config file for writing: %s\n", config_path);
        return false;
    }
    
    fprintf(file, "# AURA Simulator Configuration File\n");
    fprintf(file, "# Auto-generated - edits may be overwritten\n\n");
    
    fprintf(file, "window_width = %d\n", g_aura_config.window_width);
    fprintf(file, "window_height = %d\n", g_aura_config.window_height);
    fprintf(file, "vsync_enabled = %s\n", g_aura_config.vsync_enabled ? "true" : "false");
    fprintf(file, "log_level = %d\n", g_aura_config.log_level);
    fprintf(file, "start_fullscreen = %s\n", g_aura_config.start_fullscreen ? "true" : "false");
    fprintf(file, "theme = %s\n", g_aura_config.theme);
    fprintf(file, "auto_save_enabled = %s\n", g_aura_config.auto_save_enabled ? "true" : "false");
    fprintf(file, "auto_save_interval = %d\n", g_aura_config.auto_save_interval);
    if (g_aura_config.groq_api_key[0] != '\0') {
        fprintf(file, "groq_api_key = %s\n", g_aura_config.groq_api_key);
    }
    
    fclose(file);
    return true;
}

/**
 * Set a string configuration value
 */
bool aura_config_set_string(const char *key, const char *value) {
    if (key == NULL || value == NULL) {
        return false;
    }
    
    if (strcmp(key, "theme") == 0) {
        strncpy(g_aura_config.theme, value, sizeof(g_aura_config.theme) - 1);
        return true;
    } else if (strcmp(key, "groq_api_key") == 0) {
        strncpy(g_aura_config.groq_api_key, value, sizeof(g_aura_config.groq_api_key) - 1);
        g_aura_config.groq_api_key[sizeof(g_aura_config.groq_api_key) - 1] = '\0';
        return true;
    }
    
    return false;
}

/**
 * Set an integer configuration value
 */
bool aura_config_set_int(const char *key, int value) {
    if (key == NULL) {
        return false;
    }
    
    if (strcmp(key, "window_width") == 0) {
        g_aura_config.window_width = value;
        return true;
    } else if (strcmp(key, "window_height") == 0) {
        g_aura_config.window_height = value;
        return true;
    } else if (strcmp(key, "log_level") == 0) {
        g_aura_config.log_level = value;
        return true;
    } else if (strcmp(key, "auto_save_interval") == 0) {
        g_aura_config.auto_save_interval = value;
        return true;
    }
    
    return false;
}

/**
 * Set a boolean configuration value
 */
bool aura_config_set_bool(const char *key, bool value) {
    if (key == NULL) {
        return false;
    }
    
    if (strcmp(key, "vsync_enabled") == 0) {
        g_aura_config.vsync_enabled = value;
        return true;
    } else if (strcmp(key, "start_fullscreen") == 0) {
        g_aura_config.start_fullscreen = value;
        return true;
    } else if (strcmp(key, "auto_save_enabled") == 0) {
        g_aura_config.auto_save_enabled = value;
        return true;
    }
    
    return false;
}

/**
 * Get a string configuration value
 */
const char *aura_config_get_string(const char *key, const char *default_value) {
    if (key == NULL) {
        return default_value;
    }
    
    if (strcmp(key, "theme") == 0) {
        return g_aura_config.theme;
    } else if (strcmp(key, "app_name") == 0) {
        return g_aura_config.app_name;
    } else if (strcmp(key, "app_version") == 0) {
        return g_aura_config.app_version;
    } else if (strcmp(key, "groq_api_key") == 0) {
        return (g_aura_config.groq_api_key[0] != '\0') ? g_aura_config.groq_api_key : default_value;
    }
    
    return default_value;
}

/**
 * Get an integer configuration value
 */
int aura_config_get_int(const char *key, int default_value) {
    if (key == NULL) {
        return default_value;
    }
    
    if (strcmp(key, "window_width") == 0) {
        return g_aura_config.window_width;
    } else if (strcmp(key, "window_height") == 0) {
        return g_aura_config.window_height;
    } else if (strcmp(key, "log_level") == 0) {
        return g_aura_config.log_level;
    } else if (strcmp(key, "auto_save_interval") == 0) {
        return g_aura_config.auto_save_interval;
    }
    
    return default_value;
}

/**
 * Get a boolean configuration value
 */
bool aura_config_get_bool(const char *key, bool default_value) {
    if (key == NULL) {
        return default_value;
    }
    
    if (strcmp(key, "vsync_enabled") == 0) {
        return g_aura_config.vsync_enabled;
    } else if (strcmp(key, "start_fullscreen") == 0) {
        return g_aura_config.start_fullscreen;
    } else if (strcmp(key, "auto_save_enabled") == 0) {
        return g_aura_config.auto_save_enabled;
    }
    
    return default_value;
}

/**
 * Reset configuration to defaults
 */
void aura_config_reset_to_defaults(void) {
    printf("[AURA] Resetting configuration to defaults\n");
    memcpy(&g_aura_config, &DEFAULT_CONFIG, sizeof(AuraConfig));
}

/**
 * Cleanup configuration module
 */
void aura_config_cleanup(void) {
    printf("[AURA] Cleaning up configuration module\n");
    // Currently no special cleanup needed
}
