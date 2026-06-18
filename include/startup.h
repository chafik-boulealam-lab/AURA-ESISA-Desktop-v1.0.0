#ifndef AURA_STARTUP_H
#define AURA_STARTUP_H

#include <stdbool.h>
#include <gtk/gtk.h>

/**
 * AURA Startup Module
 * Handles application initialization sequence and splash screen
 */

typedef enum {
    STARTUP_PHASE_INIT = 0,
    STARTUP_PHASE_FILESYSTEM = 1,
    STARTUP_PHASE_ASSETS = 2,
    STARTUP_PHASE_CONFIG = 3,
    STARTUP_PHASE_DATABASE = 4,
    STARTUP_PHASE_COMPLETE = 5
} AuraStartupPhase;

typedef struct {
    GtkWidget *splash_window;
    GtkWidget *progress_bar;
    GtkWidget *status_label;
    GtkWidget *phase_label;
    AuraStartupPhase current_phase;
    bool is_complete;
} AuraStartupScreen;

/**
 * Create and display the startup splash screen
 * @return Pointer to startup screen structure
 */
AuraStartupScreen *aura_startup_create_splash(void);

/**
 * Update startup progress
 * @param startup Startup screen structure
 * @param phase Current startup phase
 * @param progress Progress percentage (0-100)
 * @param status_text Status message to display
 */
void aura_startup_update_progress(AuraStartupScreen *startup, 
                                   AuraStartupPhase phase,
                                   int progress, 
                                   const char *status_text);

/**
 * Run the complete startup sequence
 * Handles all initialization in order with visual feedback
 * @return true if startup successful, false on failure
 */
bool aura_startup_run_sequence(void);

/**
 * Close the splash screen
 * @param startup Startup screen structure
 */
void aura_startup_close_splash(AuraStartupScreen *startup);

/**
 * Destroy startup screen resources
 * @param startup Startup screen structure
 */
void aura_startup_destroy(AuraStartupScreen *startup);

/**
 * Get the startup phase name as a string
 * @param phase The startup phase
 * @return String description of the phase
 */
const char *aura_startup_get_phase_name(AuraStartupPhase phase);

#endif // AURA_STARTUP_H
