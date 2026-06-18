#ifndef AURA_ASSETS_H
#define AURA_ASSETS_H

#include <stdbool.h>
#include <stddef.h>

/**
 * AURA Assets Module
 * Manages loading and caching of application assets
 * Handles images, sounds, fonts, and other resources
 */

typedef enum {
    ASSET_TYPE_IMAGE,
    ASSET_TYPE_SOUND,
    ASSET_TYPE_FONT,
    ASSET_TYPE_CONFIG,
    ASSET_TYPE_UNKNOWN
} AuraAssetType;

/**
 * Initialize the assets module
 * Scans asset directories and prepares resource cache
 * @return true if initialization successful, false otherwise
 */
bool aura_assets_init(void);

/**
 * Load an asset from disk
 * Caches the asset in memory for future access
 * @param asset_name Name of the asset (relative path from assets/)
 * @param type Type of asset being loaded
 * @return true if asset loaded successfully
 */
bool aura_assets_load(const char *asset_name, AuraAssetType type);

/**
 * Get the full path to an asset
 * @param asset_name Name of the asset
 * @param out_path Output buffer for full path
 * @param max_len Maximum length of output buffer
 * @return true if path successfully retrieved
 */
bool aura_assets_get_path(const char *asset_name, char *out_path, size_t max_len);

/**
 * Check if an asset exists
 * @param asset_name Name of the asset
 * @return true if asset exists, false otherwise
 */
bool aura_assets_exists(const char *asset_name);

/**
 * Verify all critical assets are present
 * Called during startup to ensure application integrity
 * @return true if all critical assets present, false otherwise
 */
bool aura_assets_verify_critical(void);

/**
 * Cleanup assets module and free cached resources
 */
void aura_assets_cleanup(void);

/**
 * Get asset type from file extension
 * @param filename Name of the file
 * @return Asset type based on extension
 */
AuraAssetType aura_assets_get_type_from_extension(const char *filename);

#endif // AURA_ASSETS_H
