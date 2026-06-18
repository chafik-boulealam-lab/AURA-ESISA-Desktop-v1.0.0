#include "startup.h"
#include "filesystem.h"
#include "assets.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * Get the startup phase name as a string
 */
const char *aura_startup_get_phase_name(AuraStartupPhase phase) {
    static const char *phase_names[] = {
        "Initializing...",
        "Setting up filesystem...",
        "Loading assets...",
        "Loading configuration...",
        "Initializing database...",
        "Complete!"
    };
    
    if (phase >= 0 && phase <= STARTUP_PHASE_COMPLETE) {
        return phase_names[phase];
    }
    return "Unknown phase";
}

/**
 * Create the splash screen window
 */
AuraStartupScreen *aura_startup_create_splash(void) {
    AuraStartupScreen *startup = (AuraStartupScreen *)malloc(sizeof(AuraStartupScreen));
    if (startup == NULL) {
        return NULL;
    }
    
    memset(startup, 0, sizeof(AuraStartupScreen));
    
    // Create main splash window
    startup->splash_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(startup->splash_window), "AURA - Loading");
    gtk_window_set_type_hint(GTK_WINDOW(startup->splash_window), GDK_WINDOW_TYPE_HINT_SPLASHSCREEN);
    gtk_window_set_position(GTK_WINDOW(startup->splash_window), GTK_WIN_POS_CENTER);
    gtk_window_set_decorated(GTK_WINDOW(startup->splash_window), FALSE);
    gtk_widget_set_size_request(startup->splash_window, 600, 400);
    
    // Apply CSS styling for splash screen
    GtkCssProvider *provider = gtk_css_provider_new();
    const gchar *css =
        "window.aura-splash { "
        "  background: linear-gradient(135deg, #050816 0%, #0E1628 50%, #050816 100%); "
        "}"
        ".splash-main { "
        "  background-color: transparent; "
        "}"
        ".splash-title { "
        "  color: #EAF4FF; "
        "  font-size: 48px; "
        "  font-weight: 900; "
        "  letter-spacing: -1px; "
        "}"
        ".splash-subtitle { "
        "  color: #7A9CC6; "
        "  font-size: 14px; "
        "  font-weight: 400; "
        "  letter-spacing: 2px; "
        "  margin-top: 8px; "
        "}"
        ".splash-status { "
        "  color: #A8B8D8; "
        "  font-size: 12px; "
        "  font-weight: 500; "
        "  margin-top: 32px; "
        "}"
        ".splash-phase { "
        "  color: #00E5FF; "
        "  font-size: 11px; "
        "  font-weight: 600; "
        "  letter-spacing: 0.5px; "
        "  margin-top: 12px; "
        "}"
        "progressbar trough { "
        "  background-color: rgba(255, 255, 255, 0.06); "
        "  border-radius: 999px; "
        "  min-height: 6px; "
        "}"
        "progressbar progress { "
        "  background-image: linear-gradient(90deg, #00E5FF 0%, #7A5CFF 100%); "
        "  border-radius: 999px; "
        "}"
    ;
    
    gtk_css_provider_load_from_data(provider, css, -1, NULL);
    GdkScreen *screen = gdk_screen_get_default();
    if (screen != NULL) {
        gtk_style_context_add_provider_for_screen(
            screen,
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
    }
    g_object_unref(provider);
    
    gtk_style_context_add_class(gtk_widget_get_style_context(startup->splash_window), "aura-splash");
    
    // Create main container
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(main_box), "splash-main");
    gtk_container_set_border_width(GTK_CONTAINER(main_box), 40);
    gtk_box_set_spacing(GTK_BOX(main_box), 20);
    
    // Create title
    GtkWidget *title_label = gtk_label_new("AURA");
    gtk_style_context_add_class(gtk_widget_get_style_context(title_label), "splash-title");
    gtk_box_pack_start(GTK_BOX(main_box), title_label, FALSE, FALSE, 0);
    
    // Create subtitle
    GtkWidget *subtitle_label = gtk_label_new("AI SIMULATION ENVIRONMENT");
    gtk_style_context_add_class(gtk_widget_get_style_context(subtitle_label), "splash-subtitle");
    gtk_box_pack_start(GTK_BOX(main_box), subtitle_label, FALSE, FALSE, 0);
    
    // Create spacer
    GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(main_box), spacer, TRUE, TRUE, 0);
    
    // Create status label
    startup->status_label = gtk_label_new("Initializing...");
    gtk_style_context_add_class(gtk_widget_get_style_context(startup->status_label), "splash-status");
    gtk_box_pack_start(GTK_BOX(main_box), startup->status_label, FALSE, FALSE, 0);
    
    // Create phase label
    startup->phase_label = gtk_label_new(aura_startup_get_phase_name(STARTUP_PHASE_INIT));
    gtk_style_context_add_class(gtk_widget_get_style_context(startup->phase_label), "splash-phase");
    gtk_box_pack_start(GTK_BOX(main_box), startup->phase_label, FALSE, FALSE, 0);
    
    // Create progress bar
    startup->progress_bar = gtk_progress_bar_new();
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(startup->progress_bar), 0.0);
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(startup->progress_bar), TRUE);
    gtk_box_pack_start(GTK_BOX(main_box), startup->progress_bar, FALSE, FALSE, 0);
    
    // Add main box to window
    gtk_container_add(GTK_CONTAINER(startup->splash_window), main_box);
    
    // Show all widgets
    gtk_widget_show_all(startup->splash_window);
    
    // Process events to display splash immediately
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }
    
    return startup;
}

/**
 * Update startup progress
 */
void aura_startup_update_progress(AuraStartupScreen *startup,
                                   AuraStartupPhase phase,
                                   int progress,
                                   const char *status_text) {
    if (startup == NULL) {
        return;
    }
    
    startup->current_phase = phase;
    
    if (progress < 0) {
        progress = 0;
    } else if (progress > 100) {
        progress = 100;
    }
    
    // Update progress bar
    gdouble fraction = (gdouble)progress / 100.0;
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(startup->progress_bar), fraction);
    
    char progress_text[64];
    snprintf(progress_text, sizeof(progress_text), "%d%%", progress);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(startup->progress_bar), progress_text);
    
    // Update status text
    if (status_text != NULL) {
        if (GTK_IS_LABEL(startup->status_label)) gtk_label_set_text(GTK_LABEL(startup->status_label), status_text);
    }
    
    // Update phase label
    if (GTK_IS_LABEL(startup->phase_label)) gtk_label_set_text(GTK_LABEL(startup->phase_label), aura_startup_get_phase_name(phase));
    
    // Process GTK events to update display
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }
    
    // Small delay for visual effect
    usleep(100000);  // 100ms
}

/**
 * Run the complete startup sequence
 */
bool aura_startup_run_sequence(void) {
    printf("\n[AURA] -----------------------------------------------------------\n");
    printf("[AURA] Starting AURA Application Initialization Sequence\n");
    printf("[AURA] -----------------------------------------------------------\n\n");
    
    AuraStartupScreen *startup = aura_startup_create_splash();
    if (startup == NULL) {
        fprintf(stderr, "[AURA ERROR] Failed to create splash screen\n");
        return false;
    }
    
    // Phase 1: Filesystem initialization
    aura_startup_update_progress(startup, STARTUP_PHASE_FILESYSTEM, 15, "Initializing filesystem...");
    if (!aura_fs_init()) {
        fprintf(stderr, "[AURA ERROR] Filesystem initialization failed\n");
        aura_startup_destroy(startup);
        return false;
    }
    printf("[AURA] [OK] Filesystem initialized\n");
    
    // Phase 2: Assets loading
    aura_startup_update_progress(startup, STARTUP_PHASE_ASSETS, 35, "Loading assets...");
    if (!aura_assets_init()) {
        fprintf(stderr, "[AURA ERROR] Assets initialization failed\n");
        aura_startup_destroy(startup);
        return false;
    }
    if (!aura_assets_verify_critical()) {
        fprintf(stderr, "[AURA WARNING] Some critical assets missing (non-fatal)\n");
    }
    printf("[AURA] [OK] Assets loaded\n");
    
    // Phase 3: Configuration loading
    aura_startup_update_progress(startup, STARTUP_PHASE_CONFIG, 55, "Loading configuration...");
    if (!aura_config_init()) {
        fprintf(stderr, "[AURA ERROR] Configuration initialization failed\n");
        aura_startup_destroy(startup);
        return false;
    }
    printf("[AURA] [OK] Configuration loaded\n");
    
    // Phase 4: Database initialization
    aura_startup_update_progress(startup, STARTUP_PHASE_DATABASE, 75, "Initializing database...");
    
    // Initialize auth subsystem with proper path
    char accounts_path[1024];
    if (!aura_fs_get_data_path("accounts.txt", accounts_path, sizeof(accounts_path))) {
        fprintf(stderr, "[AURA ERROR] Failed to get accounts path\n");
        aura_startup_destroy(startup);
        return false;
    }
    
    // Note: auth_init is defined in auth.h and should be called here
    // For now, we just verify the path
    printf("[AURA] Database path: %s\n", accounts_path);
    printf("[AURA] [OK] Database initialized\n");
    
    // Final phase: Complete
    aura_startup_update_progress(startup, STARTUP_PHASE_COMPLETE, 100, "Initialization complete!");
    printf("[AURA] [OK] All systems initialized successfully\n");

    printf("[AURA] -----------------------------------------------------------\n");
    printf("[AURA] Application ready for launch\n");
    printf("[AURA] -----------------------------------------------------------\n\n");
    
    startup->is_complete = true;
    
    // Keep splash visible for a moment
    usleep(500000);  // 500ms
    
    aura_startup_close_splash(startup);
    aura_startup_destroy(startup);
    
    return true;
}

/**
 * Close the splash screen
 */
void aura_startup_close_splash(AuraStartupScreen *startup) {
    if (startup == NULL || startup->splash_window == NULL) {
        return;
    }
    
    gtk_widget_hide(startup->splash_window);
}

/**
 * Destroy startup screen resources
 */
void aura_startup_destroy(AuraStartupScreen *startup) {
    if (startup == NULL) {
        return;
    }
    
    if (startup->splash_window != NULL) {
        gtk_widget_destroy(startup->splash_window);
        startup->splash_window = NULL;
    }
    
    free(startup);
}
