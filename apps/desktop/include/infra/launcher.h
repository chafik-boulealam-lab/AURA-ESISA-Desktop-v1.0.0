#ifndef AURA_LAUNCHER_H
#define AURA_LAUNCHER_H

#include <stdbool.h>
#include <stddef.h>

/**
 * AURA Launcher Module
 * Main entry point and orchestrator for application startup
 * Coordinates filesystem, assets, config, and database initialization
 */

/**
 * Initialize all AURA subsystems
 * This is the main initialization routine called at application start
 * Handles:
 * - Filesystem setup (directories, paths)
 * - Asset loading and verification
 * - Configuration management
 * - Database initialization
 * 
 * @return true if all subsystems initialized successfully, false on error
 */
bool aura_launcher_init_all_systems(void);

/**
 * Cleanup all AURA subsystems
 * Called at application shutdown to free resources and save state
 * Handles:
 * - Config saving
 * - Asset cleanup
 * - Database cleanup
 * - Filesystem cleanup
 */
void aura_launcher_cleanup_all_systems(void);

/**
 * Get the full path to a resource
 * Resolves paths relative to application root
 * 
 * @param resource_type Type of resource (data, asset, config)
 * @param resource_name Name of the resource
 * @param out_path Output buffer for full path
 * @param max_len Maximum length of output buffer
 * @return true if path successfully resolved
 */
bool aura_launcher_get_resource_path(const char *resource_type,
                                      const char *resource_name,
                                      char *out_path,
                                      size_t max_len);

/**
 * Verify application integrity
 * Checks that all critical components are present and functional
 * 
 * @return true if application integrity verified, false otherwise
 */
bool aura_launcher_verify_integrity(void);

/**
 * Get version information
 * @return Application version string
 */
const char *aura_launcher_get_version(void);

/**
 * Get application name
 * @return Application name string
 */
const char *aura_launcher_get_app_name(void);

#endif // AURA_LAUNCHER_H
