#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "login_ui.h"
#include "session.h"
#include "auth.h"
#include "filesystem.h"
#include "aura_theme.h"

static FILE *login_debug_open(void) {
    char path[1024];
    if (aura_fs_get_data_path("login_screen_debug.log", path, sizeof(path)))
        return fopen(path, "a");
    return fopen("login_screen_debug.log", "a");
}

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

typedef struct {
    gdouble x;
    gdouble y;
    gdouble vx;
    gdouble vy;
    gdouble radius;
    gdouble alpha;
} AuraParticle;

typedef struct {
    GtkWidget *window;
    GtkWidget *background_area;
    GtkWidget *logo_area;
    GtkWidget *typing_label;
    GtkWidget *status_label;
    GtkWidget *auth_stack;
    GtkWidget *login_page;
    GtkWidget *signup_page;
    GtkWidget *recovery_page;
    GtkWidget *verification_page;
    GtkWidget *login_status_label;
    GtkWidget *login_progress_bar;
    GtkWidget *signup_status_label;
    GtkWidget *signup_progress_bar;
    GtkWidget *recovery_status_label;
    GtkWidget *signup_fullname_entry;
    GtkWidget *signup_email_entry;
    GtkWidget *signup_password_entry;
    GtkWidget *signup_confirm_entry;
    GtkWidget *return_login_button;
    GtkWidget *recovery_back_button;
    GtkWidget *verification_email_label;
    GtkWidget *verification_code_entry;
    GtkWidget *verification_status_label;
    GtkWidget *verification_confirm_button;
    GtkWidget *verification_back_button;
    GtkWidget *verification_resend_button;
    GtkWidget *verification_resend_label;
    gchar *pending_verification_email;
    GtkWidget *username_entry;
    GtkWidget *password_entry;
    GtkWidget *login_button;
    GtkWidget *exit_button;
    GtkWidget *panel; /* main login panel widget */
    GtkWidget *cover_area; /* overlay cover for fade */
    GArray *particles;
    GMainLoop *loop;
    gchar *typing_text;
    guint typing_index;
    guint background_tick_id;
    guint typing_tick_id;
    guint logo_phase;
    guint accept_close_id;
    gboolean accepted;
    /* Entry animation */
    guint entry_anim_id;
    guint entry_anim_progress; /* 0..100 */
    gdouble overlay_alpha; /* cover alpha for fade */
    gint panel_start_margin_top;
    gint panel_end_margin_top;
    gboolean components_shown;
    // AI Terminal enhancements
    GPtrArray *ai_messages;  // Array of system messages
    guint message_index;      // Current message being displayed
    guint message_phase;      // Phase counter for message animations
    GPtrArray *auth_messages; // Login verification sequence messages
    guint auth_message_index;
    guint auth_message_phase;
    guint verification_phase; // Security verification effect counter
    guint init_step_id;       // Initialization sequence timer id
    guint signup_feedback_id;
    guint signup_feedback_ticks;
    gboolean signup_feedback_success;
    guint verification_feedback_id;
    guint verification_feedback_ticks;
    gboolean verification_feedback_success;
    guint verification_show_resend_timer;
    guint verification_resend_cooldown_timer;
    guint verification_resend_show_ticks;
    guint verification_resend_cooldown_ticks;
} AuraLoginScreen;

/* Forward declarations */
static gboolean close_after_accept_cb(gpointer user_data);
static gboolean typing_tick_cb(gpointer user_data);
static gboolean signup_feedback_tick_cb(gpointer user_data);
static gboolean button_enter_notify_cb(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data);
static gboolean button_leave_notify_cb(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data);
static gboolean button_pulse_clear_cb(gpointer user_data);
static void connect_interactive_button(GtkWidget *widget);
static void play_ui_sound(GtkWidget *widget);
static void switch_auth_page(AuraLoginScreen *screen, const gchar *page_name);
static void set_feedback_state(GtkWidget *widget, gboolean success);
static void start_signup_feedback(AuraLoginScreen *screen, const gchar *message, gboolean success);
static void on_open_signup_clicked(GtkWidget *widget, gpointer user_data);
static void on_return_login_clicked(GtkWidget *widget, gpointer user_data);
static void on_open_recovery_clicked(GtkWidget *widget, gpointer user_data);
static void on_exit_clicked(GtkWidget *widget, gpointer user_data);
static void on_create_account_clicked(GtkWidget *widget, gpointer user_data);
static gboolean verification_feedback_tick_cb(gpointer user_data);
static void start_verification_feedback(AuraLoginScreen *screen, const gchar *message, gboolean success);
static void on_verify_code_clicked(GtkWidget *widget, gpointer user_data);
static void on_verification_back_clicked(GtkWidget *widget, gpointer user_data);
static gboolean verification_show_resend_timer_cb(gpointer user_data);
static gboolean verification_resend_cooldown_timer_cb(gpointer user_data);
static void on_verification_resend_clicked(GtkWidget *widget, gpointer user_data);

static void add_class(GtkWidget *widget, const char *class_name) {
    gtk_style_context_add_class(gtk_widget_get_style_context(widget), class_name);
}

static void remove_class(GtkWidget *widget, const char *class_name) {
    gtk_style_context_remove_class(gtk_widget_get_style_context(widget), class_name);
}

static void play_ui_sound(GtkWidget *widget) {
    (void)widget;
}

static void set_widget_pointer_cursor(GtkWidget *widget, gboolean active) {
    if (widget == NULL || !gtk_widget_get_realized(widget)) {
        return;
    }

    GdkWindow *gdk_window = gtk_widget_get_window(widget);
    if (gdk_window == NULL) {
        return;
    }

    if (active) {
        GdkDisplay *display = gtk_widget_get_display(widget);
        if (display != NULL) {
            GdkCursor *cursor = gdk_cursor_new_from_name(display, "pointer");
            if (cursor == NULL) {
                cursor = gdk_cursor_new_for_display(display, GDK_HAND2);
            }
            if (cursor != NULL) {
                gdk_window_set_cursor(gdk_window, cursor);
                g_object_unref(cursor);
            }
        }
    } else {
        gdk_window_set_cursor(gdk_window, NULL);
    }
}

static gboolean button_enter_notify_cb(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data) {
    (void)event;
    (void)user_data;
    play_ui_sound(widget);
    set_widget_pointer_cursor(widget, TRUE);
    return FALSE;
}

static gboolean button_leave_notify_cb(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data) {
    (void)event;
    (void)user_data;
    set_widget_pointer_cursor(widget, FALSE);
    return FALSE;
}

static gboolean button_pulse_clear_cb(gpointer user_data) {
    GtkWidget *widget = GTK_WIDGET(user_data);
    if (widget != NULL) {
        remove_class(widget, "button-pulse");
    }
    return G_SOURCE_REMOVE;
}

static void connect_interactive_button(GtkWidget *widget) {
    if (widget == NULL) {
        return;
    }

    if (GTK_IS_BUTTON(widget)) {
        gtk_button_set_relief(GTK_BUTTON(widget), GTK_RELIEF_NONE);
    }

    gtk_widget_add_events(widget, GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
    g_signal_connect(widget, "enter-notify-event", G_CALLBACK(button_enter_notify_cb), NULL);
    g_signal_connect(widget, "leave-notify-event", G_CALLBACK(button_leave_notify_cb), NULL);
}

static void pulse_button(GtkWidget *widget) {
    if (widget == NULL) {
        return;
    }

    add_class(widget, "button-pulse");
    g_timeout_add(140, button_pulse_clear_cb, widget);
}

static void apply_login_css(void) {
    GtkCssProvider *provider = gtk_css_provider_new();
    const gchar *css =
        AURA_CSS_BASE
        "window.aura-login-window { background-color: #0A0812; color: #FAF7F2; }"
        "label { color: #D8CEE8; }"
        "entry { background-color: rgba(20, 16, 31, 0.95); color: #FAF7F2; border: 1px solid rgba(139, 124, 246, 0.22); border-radius: 10px; padding: 14px; }"
        "entry:focus { border-color: #F0B429; }"
        "checkbutton label { color: #9A8FAE; }"
        ".login-subtitle { color: #9A8FAE; font-size: 14px; }"
        ".typing-label { color: #F0B429; font-size: 13px; font-weight: 600; margin-top: 16px; }"
        ".status-label { color: #8B7CF6; font-size: 12px; font-weight: 600; }"
        ".field-label { color: #D8CEE8; font-size: 11px; font-weight: 800; letter-spacing: 1.5px; }"
        ".login-entry, .name-entry {"
        "  background-color: rgba(20, 16, 31, 0.95);"
        "  color: #FAF7F2;"
        "  border: 1px solid rgba(139, 124, 246, 0.22);"
        "  border-radius: 10px;"
        "  padding: 16px;"
        "  font-size: 14px;"
        "}"
        ".login-entry:focus, .name-entry:focus { border-color: #F0B429; }"
        "button.login-button, .login-button {"
        "  background-image: linear-gradient(135deg, #F0B429 0%, #C4922A 100%);"
        "  background-color: #F0B429;"
        "  color: #1A1208;"
        "  border: none;"
        "  border-radius: 10px;"
        "  padding: 14px 20px;"
        "  font-size: 15px;"
        "  font-weight: 800;"
        "  box-shadow: 0 8px 24px rgba(240, 180, 41, 0.28);"
        "}"
        "button.login-button label, .login-button label { color: #1A1208; font-weight: 800; }"
        ".login-button:hover { background-image: linear-gradient(135deg, #F8C84A 0%, #F0B429 100%); }"
        "button.signup-button, .signup-button {"
        "  background-color: rgba(139, 124, 246, 0.15);"
        "  color: #FAF7F2;"
        "  border: 1px solid rgba(139, 124, 246, 0.35);"
        "  border-radius: 10px;"
        "  padding: 14px 20px;"
        "  font-size: 15px;"
        "  font-weight: 800;"
        "}"
        "button.signup-button label, .signup-button label { color: #FAF7F2; }"
        ".signup-button:hover { background-color: rgba(139, 124, 246, 0.28); }"
        "button.exit-button, .exit-button {"
        "  background-color: rgba(248, 113, 113, 0.1);"
        "  color: #FCA5A5;"
        "  border: 1px solid rgba(248, 113, 113, 0.25);"
        "  border-radius: 10px;"
        "  padding: 12px 20px;"
        "  font-weight: 700;"
        "}"
        "button.exit-button label, .exit-button label { color: #FCA5A5; }"
        "button.forgot-button, .forgot-button {"
        "  background-color: rgba(30, 24, 40, 0.8);"
        "  color: #F0B429;"
        "  border: 1px solid rgba(240, 180, 41, 0.25);"
        "  border-radius: 8px;"
        "  padding: 8px 12px;"
        "  font-size: 11px;"
        "  font-weight: 700;"
        "}"
        "button.forgot-button label, .forgot-button label { color: #F0B429; }"
        ".remember-check { color: #9A8FAE; font-size: 12px; font-weight: 600; }"
        ".divider-label { color: #6E6478; font-size: 11px; font-weight: 700; letter-spacing: 2px; }"
        ".stage-shell {"
        "  background-color: rgba(20, 16, 31, 0.35);"
        "  border: 1px solid rgba(240, 180, 41, 0.1);"
        "  border-radius: 28px;"
        "  box-shadow: 0 32px 80px rgba(0, 0, 0, 0.45);"
        "}"
        ".panel-card {"
        "  background-color: rgba(30, 24, 40, 0.82);"
        "  border: 1px solid rgba(139, 124, 246, 0.16);"
        "  border-radius: 22px;"
        "  padding: 32px;"
        "  box-shadow: 0 20px 56px rgba(0, 0, 0, 0.38);"
        "}"
        ".feedback-label { color: #D8CEE8; font-size: 12px; font-weight: 700; min-height: 24px; }"
        ".feedback-success { color: #34D399; }"
        ".feedback-error { color: #F87171; }"
        ".return-button {"
        "  background-color: rgba(30, 24, 40, 0.8);"
        "  color: #D8CEE8;"
        "  border: 1px solid rgba(139, 124, 246, 0.2);"
        "  border-radius: 10px;"
        "  padding: 14px 20px;"
        "  font-weight: 700;"
        "}"
        ".return-button:hover { background-color: rgba(139, 124, 246, 0.18); }"
        ".button-pulse { box-shadow: 0 0 0 4px rgba(240, 180, 41, 0.35); }"
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
}

static void initialize_particles(AuraLoginScreen *screen) {
    screen->particles = g_array_sized_new(FALSE, FALSE, sizeof(AuraParticle), 72);

    for (guint i = 0; i < 72; i++) {
        AuraParticle particle;
        particle.x = g_random_double_range(0.0, 1.0);
        particle.y = g_random_double_range(0.0, 1.0);
        particle.vx = g_random_double_range(-0.0019, 0.0019);
        particle.vy = g_random_double_range(-0.0014, 0.0014);
        particle.radius = g_random_double_range(1.2, 4.2);
        particle.alpha = g_random_double_range(0.12, 0.45);
        g_array_append_val(screen->particles, particle);
    }
}

static void draw_particle_glow(cairo_t *cr, gdouble x, gdouble y, gdouble radius, gdouble alpha, gdouble r, gdouble g, gdouble b) {
    cairo_pattern_t *glow = cairo_pattern_create_radial(x, y, 0.0, x, y, radius * 3.2);
    cairo_pattern_add_color_stop_rgba(glow, 0.0, r, g, b, alpha * 0.9);
    cairo_pattern_add_color_stop_rgba(glow, 0.4, r, g, b, alpha * 0.28);
    cairo_pattern_add_color_stop_rgba(glow, 1.0, r, g, b, 0.0);
    cairo_arc(cr, x, y, radius * 3.2, 0, 6.283185307179586);
    cairo_set_source(cr, glow);
    cairo_fill(cr);
    cairo_pattern_destroy(glow);
}

// Draw animated scanner lines effect
static void draw_scanner_lines(cairo_t *cr, gint width, gint height, guint phase) {
    gdouble scan_y = (phase % 200) * (height / 200.0);
    
    cairo_set_source_rgba(cr, 0.94, 0.71, 0.16, 0.28);
    cairo_set_line_width(cr, 1.5);
    cairo_move_to(cr, 0, scan_y);
    cairo_line_to(cr, width, scan_y);
    cairo_stroke(cr);
    
    // Glow effect above scan line
    cairo_set_source_rgba(cr, 0.55, 0.49, 0.96, 0.12);
    cairo_set_line_width(cr, 8.0);
    cairo_move_to(cr, 0, scan_y - 20);
    cairo_line_to(cr, width, scan_y - 20);
    cairo_stroke(cr);
    
    // Fading lines below (interference effect)
    for (int i = 1; i <= 3; i++) {
        gdouble fade_alpha = 0.08 / i;
        cairo_set_source_rgba(cr, 0.94, 0.71, 0.16, fade_alpha);
        cairo_set_line_width(cr, 1.0);
        cairo_move_to(cr, 0, scan_y + (i * 8));
        cairo_line_to(cr, width, scan_y + (i * 8));
        cairo_stroke(cr);
    }
}

// Draw neural access indicators
static void draw_neural_indicators(cairo_t *cr, gint width, gint height, guint phase) {
    gdouble indicator_pulse = 0.5 + (sin((phase * 2) * 3.14159 / 180.0) + 1.0) * 0.25;
    
    // Left side neural nodes
    for (int i = 0; i < 6; i++) {
        gdouble y = height * 0.1 + (i * height * 0.13);
        gdouble x = 20;
        gdouble node_size = 3.0 + (indicator_pulse * 2.0);
        
        // Outer glow
        cairo_set_source_rgba(cr, 0.94, 0.71, 0.16, 0.28 * indicator_pulse);
        cairo_arc(cr, x, y, node_size * 1.8, 0, 6.283185307179586);
        cairo_fill(cr);
        
        cairo_set_source_rgba(cr, 0.94, 0.71, 0.16, 0.65 + (indicator_pulse * 0.3));
        cairo_arc(cr, x, y, node_size * 0.8, 0, 6.283185307179586);
        cairo_fill(cr);
    }
    
    // Right side neural nodes
    for (int i = 0; i < 6; i++) {
        gdouble y = height * 0.1 + (i * height * 0.13);
        gdouble x = width - 20;
        gdouble node_size = 3.0 + (indicator_pulse * 2.0);
        
        // Outer glow
        cairo_set_source_rgba(cr, 0.48, 0.38, 1.0, 0.3 * indicator_pulse);
        cairo_arc(cr, x, y, node_size * 1.8, 0, 6.283185307179586);
        cairo_fill(cr);
        
        // Inner node
        cairo_set_source_rgba(cr, 0.48, 0.38, 1.0, 0.7 + (indicator_pulse * 0.3));
        cairo_arc(cr, x, y, node_size * 0.8, 0, 6.283185307179586);
        cairo_fill(cr);
    }
}

// Draw security verification effect
static void draw_verification_effect(cairo_t *cr, gint width, gint height, guint phase) {
    gdouble verify_progress = (phase % 120) / 120.0;
    
    // Expanding verification ring
    gdouble radius = 40.0 + (verify_progress * 80.0);
    gdouble alpha = 0.3 - (verify_progress * 0.3);
    
    cairo_set_source_rgba(cr, 0.55, 0.49, 0.96, alpha);
    cairo_set_line_width(cr, 2.0);
    cairo_arc(cr, width / 2.0, height / 2.0, radius, 0, 6.283185307179586);
    cairo_stroke(cr);
    
    gdouble corner_size = 20.0;
    cairo_set_source_rgba(cr, 0.94, 0.71, 0.16, 0.35 + (sin(verify_progress * 6.28) * 0.1));
    cairo_set_line_width(cr, 1.5);
    
    // Top-left corner
    cairo_move_to(cr, width * 0.2, height * 0.2);
    cairo_line_to(cr, width * 0.2 + corner_size, height * 0.2);
    cairo_move_to(cr, width * 0.2, height * 0.2);
    cairo_line_to(cr, width * 0.2, height * 0.2 + corner_size);
    
    // Top-right corner
    cairo_move_to(cr, width * 0.8, height * 0.2);
    cairo_line_to(cr, width * 0.8 - corner_size, height * 0.2);
    cairo_move_to(cr, width * 0.8, height * 0.2);
    cairo_line_to(cr, width * 0.8, height * 0.2 + corner_size);
    
    cairo_stroke(cr);
}

// Draw glowing AI indicators
static void draw_ai_indicators(cairo_t *cr, guint phase) {
    gdouble pulse = 0.4 + (sin(phase * 4.0 * 3.14159 / 180.0) + 1.0) * 0.3;
    
    // Top-left AI indicator
    cairo_set_source_rgba(cr, 0.94, 0.71, 0.16, pulse * 0.55);
    cairo_arc(cr, 30, 30, 8.0, 0, 6.283185307179586);
    cairo_fill(cr);
    
    cairo_set_source_rgba(cr, 0.94, 0.71, 0.16, pulse);
    cairo_arc(cr, 30, 30, 4.0, 0, 6.283185307179586);
    cairo_fill(cr);
    
    cairo_set_source_rgba(cr, 0.55, 0.49, 0.96, pulse * 0.55);
    cairo_arc(cr, 1250, 30, 8.0, 0, 6.283185307179586);
    cairo_fill(cr);
    
    cairo_set_source_rgba(cr, 0.48, 0.38, 1.0, pulse);
    cairo_arc(cr, 1250, 30, 4.0, 0, 6.283185307179586);
    cairo_fill(cr);
}

static gboolean background_draw_cb(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    AuraLoginScreen *screen = (AuraLoginScreen *)user_data;
    gint width = gtk_widget_get_allocated_width(widget);
    gint height = gtk_widget_get_allocated_height(widget);

    // Base gradient background with enhanced depth
    cairo_pattern_t *base = cairo_pattern_create_linear(0, 0, width, height);
    cairo_pattern_add_color_stop_rgb(base, 0.0, 0.07, 0.06, 0.10);
    cairo_pattern_add_color_stop_rgb(base, 0.5, 0.10, 0.09, 0.16);
    cairo_pattern_add_color_stop_rgb(base, 1.0, 0.06, 0.05, 0.09);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_set_source(cr, base);
    cairo_fill(cr);
    cairo_pattern_destroy(base);

    // Animated grid lines with parallax and glow
    gdouble grid_shift_x = fmod(screen->logo_phase * 0.3, 48.0);
    gdouble grid_shift_y = fmod(screen->logo_phase * 0.2, 42.0);
    
    // Primary grid lines (amber)
    cairo_set_source_rgba(cr, 0.91, 0.66, 0.22, 0.06);
    cairo_set_line_width(cr, 1.0);
    for (int x = 0; x <= width + 48; x += 48) {
        cairo_move_to(cr, x - grid_shift_x, 0);
        cairo_line_to(cr, x - grid_shift_x, height);
    }
    for (int y = 0; y <= height + 42; y += 42) {
        cairo_move_to(cr, 0, y - grid_shift_y);
        cairo_line_to(cr, width, y - grid_shift_y);
    }
    cairo_stroke(cr);

    // Secondary grid lines (violet, slower parallax)
    cairo_set_source_rgba(cr, 0.49, 0.42, 0.94, 0.04);
    cairo_set_line_width(cr, 0.5);
    gdouble grid_shift2_x = fmod(screen->logo_phase * 0.12, 96.0);
    gdouble grid_shift2_y = fmod(screen->logo_phase * 0.08, 84.0);
    for (int x = 24; x <= width + 96; x += 96) {
        cairo_move_to(cr, x - grid_shift2_x, 0);
        cairo_line_to(cr, x - grid_shift2_x, height);
    }
    for (int y = 21; y <= height + 84; y += 84) {
        cairo_move_to(cr, 0, y - grid_shift2_y);
        cairo_line_to(cr, width, y - grid_shift2_y);
    }
    cairo_stroke(cr);

    // Pulsing light orbs with animation
    gdouble orb_phase_1 = (gdouble)(screen->logo_phase % 80) / 80.0;
    gdouble orb_glow_1 = 0.06 + (sin(orb_phase_1 * 6.283185307179586) + 1.0) * 0.06;
    cairo_set_source_rgba(cr, 0.49, 0.42, 0.94, orb_glow_1);
    cairo_arc(cr, width * 0.78, height * 0.18, 180.0, 0, 6.283185307179586);
    cairo_fill(cr);

    gdouble orb_phase_2 = (gdouble)((screen->logo_phase + 40) % 80) / 80.0;
    gdouble orb_glow_2 = 0.05 + (sin(orb_phase_2 * 6.283185307179586) + 1.0) * 0.07;
    cairo_set_source_rgba(cr, 0.91, 0.66, 0.22, orb_glow_2);
    cairo_arc(cr, width * 0.18, height * 0.74, 220.0, 0, 6.283185307179586);
    cairo_fill(cr);

    // Additional dynamic accent orb (top right)
    gdouble orb_phase_3 = (gdouble)((screen->logo_phase + 60) % 100) / 100.0;
    gdouble orb_glow_3 = 0.04 + (sin(orb_phase_3 * 6.283185307179586) + 1.0) * 0.06;
    cairo_set_source_rgba(cr, 0.55, 0.49, 0.96, orb_glow_3);
    cairo_arc(cr, width * 0.88, height * 0.22, 160.0, 0, 6.283185307179586);
    cairo_fill(cr);

    for (guint i = 0; i < screen->particles->len; i++) {
        AuraParticle *particle = &g_array_index(screen->particles, AuraParticle, i);
        gdouble px = particle->x * width;
        gdouble py = particle->y * height;
        if (i % 3 == 0)
            draw_particle_glow(cr, px, py, particle->radius, particle->alpha, 0.94, 0.71, 0.16);
        else
            draw_particle_glow(cr, px, py, particle->radius, particle->alpha, 0.55, 0.49, 0.96);
    }

    cairo_set_source_rgba(cr, 0.94, 0.71, 0.16, 0.06);
    cairo_rectangle(cr, 32, 32, width - 64, height - 64);
    cairo_set_line_width(cr, 2.0);
    cairo_stroke(cr);

    // Corner accent glows
    gdouble corner_glow = 0.05 + (sin((screen->logo_phase * 2) * 3.14159 / 180.0) + 1.0) * 0.03;
    cairo_set_source_rgba(cr, 0.94, 0.71, 0.16, corner_glow);
    cairo_arc(cr, 40, 40, 8.0, 0, 6.283185307179586);
    cairo_fill(cr);
    cairo_arc(cr, width - 40, 40, 8.0, 0, 6.283185307179586);
    cairo_fill(cr);
    cairo_arc(cr, 40, height - 40, 8.0, 0, 6.283185307179586);
    cairo_fill(cr);
    cairo_arc(cr, width - 40, height - 40, 8.0, 0, 6.283185307179586);
    cairo_fill(cr);

    /* Glass panel illusion: semi-transparent blurred plate with neon edges and reflection */
    {
        gdouble panel_w = MIN(1120.0, width * 0.86);
        gdouble panel_h = MIN(760.0, height * 0.74);
        gdouble px = (width - panel_w) / 2.0;
        gdouble py = (height - panel_h) / 2.0;
        gdouble rr = 28.0;

        // Rounded rect path
        cairo_new_path(cr);
        cairo_move_to(cr, px + rr, py);
        cairo_line_to(cr, px + panel_w - rr, py);
        cairo_arc(cr, px + panel_w - rr, py + rr, rr, -1.5708, 0.0);
        cairo_line_to(cr, px + panel_w, py + panel_h - rr);
        cairo_arc(cr, px + panel_w - rr, py + panel_h - rr, rr, 0.0, 1.5708);
        cairo_line_to(cr, px + rr, py + panel_h);
        cairo_arc(cr, px + rr, py + panel_h - rr, rr, 1.5708, 3.14159);
        cairo_line_to(cr, px, py + rr);
        cairo_arc(cr, px + rr, py + rr, rr, 3.14159, 4.71239);

        // Fill with subtle glass gradient
        cairo_pattern_t *glass = cairo_pattern_create_linear(px, py, px, py + panel_h);
        cairo_pattern_add_color_stop_rgba(glass, 0.0, 1.0, 1.0, 1.0, 0.04);
        cairo_pattern_add_color_stop_rgba(glass, 0.12, 1.0, 1.0, 1.0, 0.02);
        cairo_pattern_add_color_stop_rgba(glass, 0.9, 0.0, 0.0, 0.0, 0.06);
        cairo_set_source(cr, glass);
        cairo_fill_preserve(cr);
        cairo_pattern_destroy(glass);

        // Neon double edge strokes
        cairo_set_line_width(cr, 1.0);
        cairo_set_source_rgba(cr, 0.94, 0.71, 0.16, 0.1);
        cairo_stroke(cr);

        cairo_new_path(cr);
        cairo_move_to(cr, px + rr, py);
        cairo_line_to(cr, px + panel_w - rr, py);
        cairo_arc(cr, px + panel_w - rr, py + rr, rr, -1.5708, 0.0);
        cairo_line_to(cr, px + panel_w, py + panel_h - rr);
        cairo_arc(cr, px + panel_w - rr, py + panel_h - rr, rr, 0.0, 1.5708);
        cairo_line_to(cr, px + rr, py + panel_h);
        cairo_arc(cr, px + rr, py + panel_h - rr, rr, 1.5708, 3.14159);
        cairo_line_to(cr, px, py + rr);
        cairo_arc(cr, px + rr, py + rr, rr, 3.14159, 4.71239);
        cairo_set_line_width(cr, 2.4);
        cairo_set_source_rgba(cr, 0.55, 0.49, 0.96, 0.08);
        cairo_stroke(cr);

        // Soft inner reflection (top-left glossy sweep)
        cairo_pattern_t *ref = cairo_pattern_create_linear(px, py, px + panel_w * 0.6, py + panel_h * 0.2);
        cairo_pattern_add_color_stop_rgba(ref, 0.0, 1.0, 1.0, 1.0, 0.12);
        cairo_pattern_add_color_stop_rgba(ref, 0.2, 1.0, 1.0, 1.0, 0.06);
        cairo_pattern_add_color_stop_rgba(ref, 0.6, 1.0, 1.0, 1.0, 0.0);
        cairo_set_source(cr, ref);
        cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
        // Clip to rounded rect and paint reflection overlay
        cairo_clip(cr);
        cairo_rectangle(cr, px, py, panel_w * 0.6, panel_h * 0.28);
        cairo_fill(cr);
        cairo_pattern_destroy(ref);

        // reset clip
        cairo_reset_clip(cr);
    }

    // AI Terminal cinematic effects
    draw_scanner_lines(cr, width, height, screen->logo_phase);
    draw_neural_indicators(cr, width, height, screen->logo_phase);
    draw_verification_effect(cr, width, height, screen->verification_phase);
    draw_ai_indicators(cr, screen->logo_phase);

    return FALSE;
}

// Top overlay draw to cover the panel during entry fade
static gboolean cover_draw_cb(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    AuraLoginScreen *screen = (AuraLoginScreen *)user_data;
    gint width = gtk_widget_get_allocated_width(widget);
    gint height = gtk_widget_get_allocated_height(widget);

    if (screen->panel == NULL) return FALSE;

    // Compute panel rect similar to background glass panel
    gdouble panel_w = MIN(1120.0, width * 0.86);
    gdouble panel_h = MIN(760.0, height * 0.74);
    gdouble px = (width - panel_w) / 2.0;
    gdouble py = (height - panel_h) / 2.0;

    // Draw a covering rect with current overlay alpha to simulate fade-in
    cairo_set_source_rgba(cr, 0.02, 0.03, 0.06, screen->overlay_alpha);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_rectangle(cr, px, py, panel_w, panel_h);
    cairo_fill(cr);

    return FALSE;
}

// Entry animation tick: fade cover alpha and slide panel up, then reveal components.
static gboolean entry_animation_tick_cb(gpointer user_data) {
    AuraLoginScreen *screen = (AuraLoginScreen *)user_data;

    if (screen->entry_anim_progress >= 100) {
        screen->entry_anim_id = 0;
        screen->overlay_alpha = 0.0;
        // ensure panel final margin
        if (screen->panel != NULL) {
            gtk_widget_set_margin_top(screen->panel, screen->panel_end_margin_top);
        }
        // Reveal components if not already
        if (!screen->components_shown) {
            if (screen->typing_label) gtk_widget_show(screen->typing_label);
            if (screen->status_label) gtk_widget_show(screen->status_label);
            if (screen->auth_stack) gtk_widget_show(screen->auth_stack);
            // start typing animation when visible
            if (screen->typing_tick_id == 0 && screen->typing_text != NULL) {
                screen->typing_tick_id = g_timeout_add(30, typing_tick_cb, screen);
            }
            screen->components_shown = TRUE;
        }
        // Remove cover area after fully hidden to avoid covering interactions
        if (screen->cover_area != NULL) {
            gtk_widget_hide(screen->cover_area);
        }
        return G_SOURCE_REMOVE;
    }

    screen->entry_anim_progress += 3; // speed
    gdouble t = screen->entry_anim_progress / 100.0;
    // ease-out cubic
    gdouble ease = 1.0 - pow(1.0 - t, 3.0);

    screen->overlay_alpha = 1.0 - ease; // from 1 -> 0

    // Slide panel from start margin down to end margin
    if (screen->panel != NULL) {
        gint mt = screen->panel_start_margin_top - (gint)((screen->panel_start_margin_top - screen->panel_end_margin_top) * ease);
        gtk_widget_set_margin_top(screen->panel, mt);
    }

    // Request redraw of cover overlay to update alpha
    if (screen->cover_area != NULL) gtk_widget_queue_draw(screen->cover_area);

    // At 40% progress reveal logo glow emphasis
    if (screen->entry_anim_progress > 40) {
        // intensify logo pulse by nudging logo_phase
        screen->logo_phase += 2;
    }

    // At 70% progress start showing components gradually
    if (screen->entry_anim_progress > 70 && !screen->components_shown) {
        if (screen->typing_label) gtk_widget_show(screen->typing_label);
        if (screen->status_label) gtk_widget_show(screen->status_label);
        if (screen->auth_stack) gtk_widget_show(screen->auth_stack);
        // start typing animation when visible
        if (screen->typing_tick_id == 0 && screen->typing_text != NULL) {
            screen->typing_tick_id = g_timeout_add(30, typing_tick_cb, screen);
        }
        screen->components_shown = TRUE;
    }

    return G_SOURCE_CONTINUE;
}

static gboolean logo_draw_cb(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    AuraLoginScreen *screen = (AuraLoginScreen *)user_data;
    gint width = gtk_widget_get_allocated_width(widget);
    gint height = gtk_widget_get_allocated_height(widget);
    
    // Enhanced pulse animation: smoother, more proportional
    gdouble pulse_progress = (gdouble)(screen->logo_phase % 40) / 40.0;
    gdouble pulse = 0.92 + (sin(pulse_progress * 6.283185307179586) + 1.0) * 0.06;

    // Clear background
    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_paint(cr);

    cairo_translate(cr, width / 2.0, height / 2.0);
    
    // Draw outer neon glow (radial gradient) - BEFORE scaling
    cairo_pattern_t *glow_pattern = cairo_pattern_create_radial(0, 0, 35, 0, 0, 62);
    cairo_pattern_add_color_stop_rgba(glow_pattern, 0.0, 0.94, 0.71, 0.16, 0.35);
    cairo_pattern_add_color_stop_rgba(glow_pattern, 0.4, 0.94, 0.71, 0.16, 0.18);
    cairo_pattern_add_color_stop_rgba(glow_pattern, 1.0, 0.94, 0.71, 0.16, 0.0);
    cairo_set_source(cr, glow_pattern);
    cairo_arc(cr, 0, 0, 62, 0, 6.283185307179586);
    cairo_fill(cr);
    cairo_pattern_destroy(glow_pattern);
    
    // Apply proportional scaling for pulsing
    cairo_scale(cr, pulse, pulse);

    // Outer cyan circle with enhanced visibility
    cairo_set_source_rgba(cr, 0.94, 0.71, 0.16, 0.55);
    cairo_arc(cr, 0, 0, 34, 0, 6.283185307179586);
    cairo_set_line_width(cr, 2.4);
    cairo_stroke(cr);
    
    cairo_set_source_rgba(cr, 0.55, 0.49, 0.96, 0.35);
    cairo_arc(cr, 0, 0, 36.5, 0, 6.283185307179586);
    cairo_set_line_width(cr, 1.2);
    cairo_stroke(cr);

    // Inner purple fill - BRIGHTER
    cairo_set_source_rgba(cr, 0.48, 0.38, 1.0, 0.55);
    cairo_arc(cr, 0, 0, 22, 0, 6.283185307179586);
    cairo_fill(cr);
    
    // Central core highlight
    cairo_set_source_rgba(cr, 0.48, 0.38, 1.0, 0.38);
    cairo_arc(cr, 0, 0, 18, 0, 6.283185307179586);
    cairo_fill(cr);

    // AURA text with gradient - FULL OPACITY
    cairo_select_font_face(cr, "Segoe UI", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 26.0);

    cairo_text_extents_t ext;
    cairo_text_extents(cr, "AURA", &ext);
    cairo_move_to(cr, -ext.width / 2.0 - ext.x_bearing, 8.0);

    cairo_pattern_t *text_gradient = cairo_pattern_create_linear(-60, -18, 60, 18);
    cairo_pattern_add_color_stop_rgba(text_gradient, 0.0, 0.98, 0.78, 0.28, 1.0);
    cairo_pattern_add_color_stop_rgba(text_gradient, 1.0, 0.55, 0.49, 0.96, 1.0);
    cairo_set_source(cr, text_gradient);
    cairo_show_text(cr, "AURA");
    cairo_pattern_destroy(text_gradient);

    // NEURAL ACCESS subtitle - BRIGHT AND VISIBLE
    cairo_select_font_face(cr, "Segoe UI", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 10.0);
    cairo_set_source_rgba(cr, 0.75, 0.85, 1.0, 1.0);
    cairo_text_extents(cr, "NEURAL ACCESS", &ext);
    cairo_move_to(cr, -ext.width / 2.0 - ext.x_bearing, 28.0);
    cairo_show_text(cr, "ESISA");

    return FALSE;
}

static void update_ai_status_message(AuraLoginScreen *screen) {
    if (screen->ai_messages == NULL || screen->ai_messages->len == 0) {
        return;
    }
    
    if (screen->status_label == NULL) {
        return;
    }
    
    // Defensive type check - verify it's actually a label before using it
    // This check must come before any label operations
    if (!G_TYPE_CHECK_INSTANCE_TYPE(G_OBJECT(screen->status_label), GTK_TYPE_LABEL)) {
        static gboolean logged_once = FALSE;
        if (!logged_once) {
            fprintf(stderr, "[ERROR] status_label is not a GtkLabel, type=%s\n", 
                    G_OBJECT_TYPE_NAME(screen->status_label));
            logged_once = TRUE;
        }
        return;
    }
    
    // Cycle through messages every 120 frames (~7.5 seconds at 16ms ticks)
    guint message_index = (screen->logo_phase / 120) % screen->ai_messages->len;
    guint phase_within_message = screen->logo_phase % 120;
    
    if (message_index != screen->message_index) {
        screen->message_index = message_index;
        screen->message_phase = 0;
    } else {
        screen->message_phase++;
    }
    
    gchar *msg = (gchar *)g_ptr_array_index(screen->ai_messages, message_index);
    int progress_idx = MIN(phase_within_message / 9, 12);
    if (progress_idx > 0) progress_idx--;
    gchar progress_char = "|/-\\"[phase_within_message % 4];  // Spinner
    gchar *status_text = g_strdup_printf("[%c] %s", progress_char, msg);

    gtk_label_set_text(GTK_LABEL(screen->status_label), status_text);
    g_free(status_text);
}

// Initialization sequence steps when user clicks Login
static gboolean initialization_step_cb(gpointer user_data) {
    AuraLoginScreen *screen = (AuraLoginScreen *)user_data;

    if (screen->auth_messages == NULL || screen->auth_messages->len == 0) {
        screen->init_step_id = 0;
        return G_SOURCE_REMOVE;
    }

    const guint ticks_per_message = 5;

    if (screen->auth_message_index >= screen->auth_messages->len) {
        screen->init_step_id = 0;
        if (GTK_IS_LABEL(screen->login_status_label)) {
            gtk_label_set_text(GTK_LABEL(screen->login_status_label), "Acces autorise. Chargement de l'espace AURA...");
            set_feedback_state(screen->login_status_label, TRUE);
        }
        if (GTK_IS_PROGRESS_BAR(screen->login_progress_bar)) {
            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(screen->login_progress_bar), 1.0);
            gtk_progress_bar_set_text(GTK_PROGRESS_BAR(screen->login_progress_bar), "Espace pret");
        }

        if (screen->accept_close_id == 0) {
            screen->accept_close_id = g_timeout_add(850, close_after_accept_cb, screen);
        }

        return G_SOURCE_REMOVE;
    }

    const char spinner[] = "|/-\\";
    char spin = spinner[screen->auth_message_phase % 4];
    gchar *msg = (gchar *)g_ptr_array_index(screen->auth_messages, screen->auth_message_index);
    gchar *txt = g_strdup_printf("[%c] %s", spin, msg);

    if (GTK_IS_LABEL(screen->login_status_label)) {
        gtk_label_set_text(GTK_LABEL(screen->login_status_label), txt);
        set_feedback_state(screen->login_status_label, TRUE);
    }
    if (GTK_IS_PROGRESS_BAR(screen->login_progress_bar)) {
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(screen->login_progress_bar), txt);
    }

    gdouble progress = ((gdouble)screen->auth_message_index + ((gdouble)screen->auth_message_phase / ticks_per_message)) / (gdouble)screen->auth_messages->len;
    if (GTK_IS_PROGRESS_BAR(screen->login_progress_bar)) {
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(screen->login_progress_bar), progress);
    }
    g_free(txt);

    screen->verification_phase += 6;
    screen->auth_message_phase++;
    if (screen->auth_message_phase >= ticks_per_message) {
        screen->auth_message_phase = 0;
        screen->auth_message_index++;
    }

    return G_SOURCE_CONTINUE;
}

static gboolean animation_tick_cb(gpointer user_data) {
    AuraLoginScreen *screen = (AuraLoginScreen *)user_data;

    if (screen->background_area == NULL || screen->logo_area == NULL) {
        screen->background_tick_id = 0;
        return G_SOURCE_REMOVE;
    }

    screen->logo_phase++;
    screen->verification_phase++;
    
    // Update AI status message (function has defensive type checks)
    if (screen->status_label != NULL && gtk_widget_get_realized(screen->status_label)) {
        update_ai_status_message(screen);
    }

    if (screen->particles != NULL) {
        for (guint i = 0; i < screen->particles->len; i++) {
            AuraParticle *particle = &g_array_index(screen->particles, AuraParticle, i);
            particle->x += particle->vx;
            particle->y += particle->vy;

            if (particle->x < -0.05) {
                particle->x = 1.05;
            } else if (particle->x > 1.05) {
                particle->x = -0.05;
            }

            if (particle->y < -0.05) {
                particle->y = 1.05;
            } else if (particle->y > 1.05) {
                particle->y = -0.05;
            }
        }
    }

    gtk_widget_queue_draw(screen->background_area);
    gtk_widget_queue_draw(screen->logo_area);
    return G_SOURCE_CONTINUE;
}

static gboolean typing_tick_cb(gpointer user_data) {
    AuraLoginScreen *screen = (AuraLoginScreen *)user_data;

    if (screen->typing_label == NULL) {
        screen->typing_tick_id = 0;
        return G_SOURCE_REMOVE;
    }

    gsize full_len = strlen(screen->typing_text);

    if (screen->typing_index >= full_len) {
        gtk_label_set_text(GTK_LABEL(screen->typing_label), screen->typing_text);
        screen->typing_tick_id = 0;
        return G_SOURCE_REMOVE;
    }

    screen->typing_index++;
    gchar *partial = g_strndup(screen->typing_text, screen->typing_index);
    gtk_label_set_text(GTK_LABEL(screen->typing_label), partial);
    g_free(partial);

    return G_SOURCE_CONTINUE;
}

static void set_feedback_state(GtkWidget *widget, gboolean success) {
    if (widget == NULL) {
        return;
    }

    remove_class(widget, "feedback-success");
    remove_class(widget, "feedback-error");
    add_class(widget, success ? "feedback-success" : "feedback-error");
}

static void switch_auth_page(AuraLoginScreen *screen, const gchar *page_name) {
    if (screen == NULL || screen->auth_stack == NULL || page_name == NULL) {
        return;
    }

    gtk_stack_set_visible_child_name(GTK_STACK(screen->auth_stack), page_name);

    if (g_strcmp0(page_name, "login") == 0) {
        gtk_label_set_text(GTK_LABEL(screen->login_status_label), "");
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(screen->login_progress_bar), 0.0);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(screen->login_progress_bar), "");
        if (screen->username_entry != NULL) {
            gtk_widget_grab_focus(screen->username_entry);
        }
    } else if (g_strcmp0(page_name, "signup") == 0) {
        gtk_label_set_text(GTK_LABEL(screen->signup_status_label), "");
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(screen->signup_progress_bar), 0.0);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(screen->signup_progress_bar), "");
        if (screen->signup_fullname_entry != NULL) {
            gtk_widget_grab_focus(screen->signup_fullname_entry);
        }
    } else if (g_strcmp0(page_name, "recovery") == 0) {
        if (screen->recovery_status_label != NULL) {
            gtk_label_set_text(GTK_LABEL(screen->recovery_status_label), "Password recovery unavailable in offline mode.");
        }
        if (screen->recovery_back_button != NULL) {
            gtk_widget_grab_focus(screen->recovery_back_button);
        }
    }
}

static void start_signup_feedback(AuraLoginScreen *screen, const gchar *message, gboolean success) {
    if (screen == NULL) {
        return;
    }

    screen->signup_feedback_success = success;
    screen->signup_feedback_ticks = 0;

    if (screen->signup_status_label != NULL) {
        gtk_label_set_text(GTK_LABEL(screen->signup_status_label), message);
        set_feedback_state(screen->signup_status_label, success);
    }

    if (screen->signup_progress_bar != NULL) {
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(screen->signup_progress_bar), 0.0);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(screen->signup_progress_bar), success ? "Creating account" : "Validation error");
    }

    if (screen->signup_feedback_id == 0) {
        if (screen->auth_stack != NULL) {
            gtk_widget_set_sensitive(screen->auth_stack, FALSE);
        }
        screen->signup_feedback_id = g_timeout_add(35, signup_feedback_tick_cb, screen);
    }
}

static gboolean signup_feedback_tick_cb(gpointer user_data) {
    AuraLoginScreen *screen = (AuraLoginScreen *)user_data;

    if (screen == NULL || screen->signup_progress_bar == NULL || screen->signup_status_label == NULL) {
        if (screen != NULL) {
            screen->signup_feedback_id = 0;
        }
        return G_SOURCE_REMOVE;
    }

    screen->signup_feedback_ticks++;

    if (screen->signup_feedback_success) {
        gdouble fraction = (gdouble)screen->signup_feedback_ticks / 20.0;
        if (fraction > 1.0) {
            fraction = 1.0;
        }
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(screen->signup_progress_bar), fraction);

        if (fraction >= 1.0) {
            const gchar *created_email = gtk_entry_get_text(GTK_ENTRY(screen->signup_email_entry));
            gtk_label_set_text(GTK_LABEL(screen->signup_status_label), "Account created successfully. Returning to login...");
            gtk_progress_bar_set_text(GTK_PROGRESS_BAR(screen->signup_progress_bar), "Account created");

            gtk_entry_set_text(GTK_ENTRY(screen->username_entry), created_email);
            gtk_entry_set_text(GTK_ENTRY(screen->password_entry), "");
            gtk_entry_set_text(GTK_ENTRY(screen->signup_fullname_entry), "");
            gtk_entry_set_text(GTK_ENTRY(screen->signup_email_entry), "");
            gtk_entry_set_text(GTK_ENTRY(screen->signup_password_entry), "");
            gtk_entry_set_text(GTK_ENTRY(screen->signup_confirm_entry), "");

            screen->signup_feedback_id = 0;
            if (screen->auth_stack != NULL) {
                gtk_widget_set_sensitive(screen->auth_stack, TRUE);
            }
            switch_auth_page(screen, "login");
            return G_SOURCE_REMOVE;
        }
    } else {
        gtk_progress_bar_pulse(GTK_PROGRESS_BAR(screen->signup_progress_bar));
        if (screen->signup_feedback_ticks > 22) {
            screen->signup_feedback_id = 0;
            if (screen->auth_stack != NULL) {
                gtk_widget_set_sensitive(screen->auth_stack, TRUE);
            }
            return G_SOURCE_REMOVE;
        }
    }

    return G_SOURCE_CONTINUE;
}

static void on_open_signup_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AuraLoginScreen *screen = (AuraLoginScreen *)user_data;
    play_ui_sound(widget);
    pulse_button(widget);
    switch_auth_page(screen, "signup");
}

static void on_return_login_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AuraLoginScreen *screen = (AuraLoginScreen *)user_data;
    play_ui_sound(widget);
    pulse_button(widget);
    switch_auth_page(screen, "login");
}

static void on_open_recovery_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AuraLoginScreen *screen = (AuraLoginScreen *)user_data;
    play_ui_sound(widget);
    pulse_button(widget);
    switch_auth_page(screen, "recovery");
}

static void on_exit_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AuraLoginScreen *screen = (AuraLoginScreen *)user_data;
    play_ui_sound(widget);
    pulse_button(widget);
    if (screen != NULL && screen->window != NULL) {
        gtk_widget_destroy(screen->window);
    }
}

/* ===== VERIFICATION FEEDBACK ===== */
static void start_verification_feedback(AuraLoginScreen *screen, const gchar *message, gboolean success) {
    if (screen == NULL) {
        return;
    }

    screen->verification_feedback_success = success;
    screen->verification_feedback_ticks = 0;

    if (screen->verification_status_label != NULL) {
        gtk_label_set_text(GTK_LABEL(screen->verification_status_label), message);
        set_feedback_state(screen->verification_status_label, success);
    }

    if (screen->verification_feedback_id == 0) {
        if (screen->auth_stack != NULL) {
            gtk_widget_set_sensitive(screen->auth_stack, FALSE);
        }
        screen->verification_feedback_id = g_timeout_add(35, verification_feedback_tick_cb, screen);
    }
}

static gboolean verification_feedback_tick_cb(gpointer user_data) {
    AuraLoginScreen *screen = (AuraLoginScreen *)user_data;

    if (screen == NULL || screen->verification_status_label == NULL) {
        if (screen != NULL) {
            screen->verification_feedback_id = 0;
        }
        return G_SOURCE_REMOVE;
    }

    screen->verification_feedback_ticks++;

    if (screen->verification_feedback_success) {
        /* Success case - verified, return to login */
        if (screen->verification_feedback_ticks >= 10) {
            gtk_label_set_text(GTK_LABEL(screen->verification_status_label), "Email verified successfully. Returning to login...");

            /* Store email in login entry */
            if (screen->pending_verification_email != NULL) {
                gtk_entry_set_text(GTK_ENTRY(screen->username_entry), screen->pending_verification_email);
                g_free(screen->pending_verification_email);
                screen->pending_verification_email = NULL;
            }

            gtk_entry_set_text(GTK_ENTRY(screen->verification_code_entry), "");

            screen->verification_feedback_id = 0;
            if (screen->auth_stack != NULL) {
                gtk_widget_set_sensitive(screen->auth_stack, TRUE);
            }
            switch_auth_page(screen, "login");
            return G_SOURCE_REMOVE;
        }
    } else {
        /* Error case - pulse and then allow retry */
        if (screen->verification_feedback_ticks > 30) {
            screen->verification_feedback_id = 0;
            if (screen->auth_stack != NULL) {
                gtk_widget_set_sensitive(screen->auth_stack, TRUE);
            }
            gtk_entry_set_text(GTK_ENTRY(screen->verification_code_entry), "");
            gtk_widget_grab_focus(GTK_WIDGET(screen->verification_code_entry));
            return G_SOURCE_REMOVE;
        }
    }

    return G_SOURCE_CONTINUE;
}

static void on_verify_code_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AuraLoginScreen *screen = (AuraLoginScreen *)user_data;
    play_ui_sound(widget);
    pulse_button(widget);

    const gchar *code = gtk_entry_get_text(GTK_ENTRY(screen->verification_code_entry));
    char errbuf[256];

    if (code == NULL || code[0] == '\0') {
        start_verification_feedback(screen, "Please enter the verification code.", FALSE);
        return;
    }

    if (strlen(code) != 6) {
        start_verification_feedback(screen, "Verification code must be 6 digits.", FALSE);
        return;
    }

    if (screen->pending_verification_email == NULL) {
        start_verification_feedback(screen, "No pending verification.", FALSE);
        return;
    }

    if (!auth_verify_code(screen->pending_verification_email, code, errbuf, sizeof(errbuf))) {
        start_verification_feedback(screen, errbuf, FALSE);
        return;
    }

    start_verification_feedback(screen, "Code verified successfully!", TRUE);
}

static void on_verification_back_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AuraLoginScreen *screen = (AuraLoginScreen *)user_data;
    play_ui_sound(widget);
    pulse_button(widget);

    gtk_entry_set_text(GTK_ENTRY(screen->verification_code_entry), "");
    gtk_label_set_text(GTK_LABEL(screen->verification_status_label), "");

    if (screen->pending_verification_email != NULL) {
        g_free(screen->pending_verification_email);
        screen->pending_verification_email = NULL;
    }

    /* Stop timers */
    if (screen->verification_show_resend_timer != 0) {
        g_source_remove(screen->verification_show_resend_timer);
        screen->verification_show_resend_timer = 0;
    }
    if (screen->verification_resend_cooldown_timer != 0) {
        g_source_remove(screen->verification_resend_cooldown_timer);
        screen->verification_resend_cooldown_timer = 0;
    }

    switch_auth_page(screen, "login");
}

/* ===== RESEND CODE TIMER (Show resend button after 60 seconds) ===== */
static gboolean verification_show_resend_timer_cb(gpointer user_data) {
    AuraLoginScreen *screen = (AuraLoginScreen *)user_data;

    if (screen == NULL || screen->verification_resend_button == NULL) {
        if (screen != NULL) {
            screen->verification_show_resend_timer = 0;
        }
        return G_SOURCE_REMOVE;
    }

    screen->verification_resend_show_ticks++;

    /* Show button after 60 seconds (1714 ticks at 35ms) */
    if (screen->verification_resend_show_ticks >= 1714) {
        gtk_widget_set_sensitive(screen->verification_resend_button, TRUE);
        gtk_button_set_label(GTK_BUTTON(screen->verification_resend_button), "Resend Code");
        screen->verification_show_resend_timer = 0;
        return G_SOURCE_REMOVE;
    }

    /* Update button label with countdown */
    gint remaining = 60 - (screen->verification_resend_show_ticks / 28);
    if (remaining < 1) remaining = 1;
    gchar label[64];
    snprintf(label, sizeof(label), "Resend Code (wait %ds)", remaining);
    gtk_button_set_label(GTK_BUTTON(screen->verification_resend_button), label);

    return G_SOURCE_CONTINUE;
}

/* ===== RESEND COOLDOWN TIMER (10 minute cooldown after resend) ===== */
static gboolean verification_resend_cooldown_timer_cb(gpointer user_data) {
    AuraLoginScreen *screen = (AuraLoginScreen *)user_data;

    if (screen == NULL || screen->verification_resend_button == NULL) {
        if (screen != NULL) {
            screen->verification_resend_cooldown_timer = 0;
        }
        return G_SOURCE_REMOVE;
    }

    screen->verification_resend_cooldown_ticks++;

    /* 10 minutes = 600 seconds (~17143 ticks at 35ms) */
    if (screen->verification_resend_cooldown_ticks >= 17143) {
        gtk_widget_set_sensitive(screen->verification_resend_button, TRUE);
        gtk_button_set_label(GTK_BUTTON(screen->verification_resend_button), "Resend Code");
        screen->verification_resend_cooldown_timer = 0;
        return G_SOURCE_REMOVE;
    }

    /* Update button label with countdown (show remaining time) */
    gint total_seconds = 600;
    gint elapsed_seconds = screen->verification_resend_cooldown_ticks / 28;
    gint remaining_seconds = total_seconds - elapsed_seconds;
    if (remaining_seconds < 1) remaining_seconds = 1;

    gint mins = remaining_seconds / 60;
    gint secs = remaining_seconds % 60;

    gchar label[64];
    snprintf(label, sizeof(label), "Resend in %d:%02d", mins, secs);
    gtk_button_set_label(GTK_BUTTON(screen->verification_resend_button), label);

    return G_SOURCE_CONTINUE;
}

static void on_verification_resend_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AuraLoginScreen *screen = (AuraLoginScreen *)user_data;
    play_ui_sound(widget);
    pulse_button(widget);

    if (screen->pending_verification_email == NULL) {
        start_verification_feedback(screen, "No email to resend code to.", FALSE);
        return;
    }

    /* Generate and send new code */
    char errbuf[256];
    unsigned int generated_code = 0;
    (void)generated_code; /* Suppress unused variable warning in production */
    if (!auth_send_verification_code(screen->pending_verification_email, &generated_code, errbuf, sizeof(errbuf))) {
        start_verification_feedback(screen, "Failed to resend code. Try again or contact support.", FALSE);
        return;
    }

    /* Disable resend button and start 10-minute cooldown */
    gtk_widget_set_sensitive(screen->verification_resend_button, FALSE);
    gtk_button_set_label(GTK_BUTTON(screen->verification_resend_button), "Resend in 10:00");

    screen->verification_resend_cooldown_ticks = 0;
    if (screen->verification_resend_cooldown_timer != 0) {
        g_source_remove(screen->verification_resend_cooldown_timer);
    }
    screen->verification_resend_cooldown_timer = g_timeout_add(35, verification_resend_cooldown_timer_cb, screen);

    start_verification_feedback(screen, "Verification code resent to your email!", TRUE);
}

static void on_create_account_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AuraLoginScreen *screen = (AuraLoginScreen *)user_data;
    play_ui_sound(widget);
    pulse_button(widget);
    const gchar *fullname = gtk_entry_get_text(GTK_ENTRY(screen->signup_fullname_entry));
    const gchar *gmail = gtk_entry_get_text(GTK_ENTRY(screen->signup_email_entry));
    const gchar *password = gtk_entry_get_text(GTK_ENTRY(screen->signup_password_entry));
    const gchar *confirm = gtk_entry_get_text(GTK_ENTRY(screen->signup_confirm_entry));
    char errbuf[256];

    if (fullname == NULL || fullname[0] == '\0' || gmail == NULL || gmail[0] == '\0' || password == NULL || password[0] == '\0' || confirm == NULL || confirm[0] == '\0') {
        start_signup_feedback(screen, "All fields are required to create an account.", FALSE);
        return;
    }

    if (strcmp(password, confirm) != 0) {
        start_signup_feedback(screen, "Passwords do not match.", FALSE);
        return;
    }

    if (!auth_create_account(fullname, gmail, password, errbuf, sizeof(errbuf))) {
        start_signup_feedback(screen, errbuf, FALSE);
        return;
    }

    start_signup_feedback(screen, "Account created successfully. You can sign in now.", TRUE);
}

static gboolean close_after_accept_cb(gpointer user_data) {
    AuraLoginScreen *screen = (AuraLoginScreen *)user_data;

    screen->accept_close_id = 0;
    if (screen->window != NULL) {
        gtk_widget_destroy(screen->window);
    }

    return G_SOURCE_REMOVE;
}

static void on_window_destroy(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AuraLoginScreen *screen = (AuraLoginScreen *)user_data;
    
    FILE *dbg = login_debug_open();
    fprintf(dbg, "[DESTROY] Window destroy event triggered\n");
    fflush(dbg);
    fclose(dbg);

    if (screen->background_tick_id != 0) {
        guint background_id = screen->background_tick_id;
        screen->background_tick_id = 0;
        g_source_remove(background_id);
    }
    if (screen->typing_tick_id != 0) {
        guint typing_id = screen->typing_tick_id;
        screen->typing_tick_id = 0;
        g_source_remove(typing_id);
    }
    if (screen->accept_close_id != 0) {
        guint close_id = screen->accept_close_id;
        screen->accept_close_id = 0;
        g_source_remove(close_id);
    }
    if (screen->init_step_id != 0) {
        guint init_id = screen->init_step_id;
        screen->init_step_id = 0;
        g_source_remove(init_id);
    }
    if (screen->signup_feedback_id != 0) {
        guint signup_feedback_id = screen->signup_feedback_id;
        screen->signup_feedback_id = 0;
        g_source_remove(signup_feedback_id);
    }
    if (screen->verification_show_resend_timer != 0) {
        guint resend_show_timer = screen->verification_show_resend_timer;
        screen->verification_show_resend_timer = 0;
        g_source_remove(resend_show_timer);
    }
    if (screen->verification_resend_cooldown_timer != 0) {
        guint resend_cooldown_timer = screen->verification_resend_cooldown_timer;
        screen->verification_resend_cooldown_timer = 0;
        g_source_remove(resend_cooldown_timer);
    }

    if (screen->loop != NULL) {
        g_main_loop_quit(screen->loop);
    }
}

static void on_login_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AuraLoginScreen *screen = (AuraLoginScreen *)user_data;
    play_ui_sound(widget);
    pulse_button(widget);
    const gchar *username = gtk_entry_get_text(GTK_ENTRY(screen->username_entry));
    const gchar *password = gtk_entry_get_text(GTK_ENTRY(screen->password_entry));

    if (username == NULL || username[0] == '\0' || password == NULL || password[0] == '\0') {
        gtk_label_set_text(GTK_LABEL(screen->login_status_label), "Email and password are required.");
        set_feedback_state(screen->login_status_label, FALSE);
        return;
    }

    AuraAccount acct;
    memset(&acct, 0, sizeof(acct));
    if (!auth_verify_credentials(username, password, &acct)) {
        gtk_label_set_text(GTK_LABEL(screen->login_status_label), "Acces refuse. Email ou mot de passe invalide.");
        set_feedback_state(screen->login_status_label, FALSE);
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(screen->login_progress_bar), 0.0);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(screen->login_progress_bar), "Denied");
        return;
    }

    aura_session_set(acct.fullname, acct.gmail);
    screen->accepted = TRUE;
    gtk_widget_set_sensitive(screen->auth_stack, FALSE);
    gtk_label_set_text(GTK_LABEL(screen->login_status_label), "Verification des identifiants...");
    set_feedback_state(screen->login_status_label, TRUE);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(screen->login_progress_bar), 0.0);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(screen->login_progress_bar), "Verification");
    screen->auth_message_index = 0;
    screen->auth_message_phase = 0;
    screen->verification_phase = 0;
    if (screen->init_step_id == 0) {
        screen->init_step_id = g_timeout_add(300, initialization_step_cb, screen);
    }
}

static void destroy_login_screen(AuraLoginScreen *screen) {
    if (screen == NULL) {
        return;
    }

    if (screen->background_tick_id != 0) {
        guint background_id = screen->background_tick_id;
        screen->background_tick_id = 0;
        g_source_remove(background_id);
    }
    if (screen->typing_tick_id != 0) {
        guint typing_id = screen->typing_tick_id;
        screen->typing_tick_id = 0;
        g_source_remove(typing_id);
    }
    if (screen->accept_close_id != 0) {
        guint close_id = screen->accept_close_id;
        screen->accept_close_id = 0;
        g_source_remove(close_id);
    }

    if (screen->particles != NULL) {
        g_array_unref(screen->particles);
    }
    g_clear_pointer(&screen->typing_text, g_free);
    if (screen->ai_messages != NULL) {
        g_ptr_array_unref(screen->ai_messages);
    }
    if (screen->auth_messages != NULL) {
        g_ptr_array_unref(screen->auth_messages);
    }
    if (screen->signup_feedback_id != 0) {
        guint signup_feedback_id = screen->signup_feedback_id;
        screen->signup_feedback_id = 0;
        g_source_remove(signup_feedback_id);
    }
    if (screen->verification_feedback_id != 0) {
        guint verification_feedback_id = screen->verification_feedback_id;
        screen->verification_feedback_id = 0;
        g_source_remove(verification_feedback_id);
    }
    if (screen->pending_verification_email != NULL) {
        g_free(screen->pending_verification_email);
        screen->pending_verification_email = NULL;
    }
    if (screen->verification_show_resend_timer != 0) {
        guint resend_show_timer = screen->verification_show_resend_timer;
        screen->verification_show_resend_timer = 0;
        g_source_remove(resend_show_timer);
    }
    if (screen->verification_resend_cooldown_timer != 0) {
        guint resend_cooldown_timer = screen->verification_resend_cooldown_timer;
        screen->verification_resend_cooldown_timer = 0;
        g_source_remove(resend_cooldown_timer);
    }
    if (screen->loop != NULL) {
        g_main_loop_unref(screen->loop);
    }

    g_free(screen);
}

gboolean aura_launch_login_screen(int *argc, char ***argv) {
    FILE *dbg = login_debug_open();
    fprintf(dbg, "[LOGIN] Starting aura_launch_login_screen\n");
    fflush(dbg);
    
    (void)argc;
    (void)argv;

    fprintf(dbg, "[LOGIN] Calling apply_login_css\n");
    fflush(dbg);
    apply_login_css();

    fprintf(dbg, "[LOGIN] Creating AuraLoginScreen\n");
    fflush(dbg);
    AuraLoginScreen *screen = g_new0(AuraLoginScreen, 1);
    screen->typing_text = g_strdup("Initialisation de l'environnement AURA...");
    screen->loop = g_main_loop_new(NULL, FALSE);
    initialize_particles(screen);
    
    fprintf(dbg, "[LOGIN] Initializing AI messages\n");
    fflush(dbg);
    // Initialize AI system messages
    screen->ai_messages = g_ptr_array_new_with_free_func(g_free);
    g_ptr_array_add(screen->ai_messages, g_strdup("Initialisation du noyau IA..."));
    g_ptr_array_add(screen->ai_messages, g_strdup("Systemes neuronaux en ligne"));
    g_ptr_array_add(screen->ai_messages, g_strdup("Verification d'acces prete"));
    g_ptr_array_add(screen->ai_messages, g_strdup("Connexion securisee etablie"));
    screen->message_index = 0;
    screen->message_phase = 0;

    screen->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(screen->window), "AURA — Connexion ESISA");
    gtk_window_set_default_size(GTK_WINDOW(screen->window), 1280, 920);
    gtk_window_set_position(GTK_WINDOW(screen->window), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable(GTK_WINDOW(screen->window), TRUE);
    add_class(screen->window, "aura-login-window");
    g_signal_connect(screen->window, "destroy", G_CALLBACK(on_window_destroy), screen);

    GtkWidget *overlay = gtk_overlay_new();
    gtk_container_add(GTK_CONTAINER(screen->window), overlay);

    screen->background_area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(screen->background_area, TRUE);
    gtk_widget_set_vexpand(screen->background_area, TRUE);
    g_signal_connect(screen->background_area, "draw", G_CALLBACK(background_draw_cb), screen);
    g_object_add_weak_pointer(G_OBJECT(screen->background_area), (gpointer *)&screen->background_area);
    gtk_container_add(GTK_CONTAINER(overlay), screen->background_area);

    // CENTERED CONTAINER for two-panel layout
    GtkWidget *center_container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(center_container, TRUE);
    gtk_widget_set_vexpand(center_container, TRUE);
    gtk_widget_set_halign(center_container, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(center_container, GTK_ALIGN_CENTER);
    gtk_widget_set_size_request(center_container, 1120, 760);
    gtk_box_set_spacing(GTK_BOX(center_container), 28);
    add_class(center_container, "stage-shell");
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), center_container);

    // LEFT PANEL: AI Visuals and Logo
    GtkWidget *left_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 24);
    gtk_widget_set_size_request(left_panel, 520, 720);
    gtk_widget_set_halign(left_panel, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(left_panel, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_start(left_panel, 32);
    gtk_widget_set_margin_end(left_panel, 32);
    gtk_widget_set_margin_top(left_panel, 60);
    gtk_widget_set_margin_bottom(left_panel, 60);
    gtk_box_pack_start(GTK_BOX(center_container), left_panel, FALSE, FALSE, 0);

    // Logo area in left panel
    screen->logo_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(screen->logo_area, 320, 320);
    gtk_widget_set_halign(screen->logo_area, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(screen->logo_area, GTK_ALIGN_START);
    add_class(screen->logo_area, "logo-shell");
    g_signal_connect(screen->logo_area, "draw", G_CALLBACK(logo_draw_cb), screen);
    g_object_add_weak_pointer(G_OBJECT(screen->logo_area), (gpointer *)&screen->logo_area);
    gtk_box_pack_start(GTK_BOX(left_panel), screen->logo_area, FALSE, FALSE, 0);

    // AI status messages in left panel
    screen->status_label = gtk_label_new("Initialisation du noyau IA...");
    gtk_label_set_xalign(GTK_LABEL(screen->status_label), 0.5f);
    gtk_label_set_line_wrap(GTK_LABEL(screen->status_label), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(screen->status_label), 42);
    add_class(screen->status_label, "status-label");
    gtk_widget_set_hexpand(screen->status_label, TRUE);
    gtk_widget_hide(screen->status_label);
    gtk_box_pack_start(GTK_BOX(left_panel), screen->status_label, FALSE, FALSE, 0);

    screen->typing_label = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(screen->typing_label), 0.5f);
    add_class(screen->typing_label, "typing-label");
    g_object_add_weak_pointer(G_OBJECT(screen->typing_label), (gpointer *)&screen->typing_label);
    gtk_widget_set_hexpand(screen->typing_label, TRUE);
    gtk_widget_hide(screen->typing_label);
    gtk_box_pack_start(GTK_BOX(left_panel), screen->typing_label, FALSE, FALSE, 0);

    // RIGHT PANEL: Authentication stack with login and sign-up pages
    screen->auth_stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(screen->auth_stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    gtk_stack_set_transition_duration(GTK_STACK(screen->auth_stack), 320);
    gtk_widget_set_size_request(screen->auth_stack, 470, 720);
    gtk_widget_set_halign(screen->auth_stack, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(screen->auth_stack, GTK_ALIGN_CENTER);
    add_class(screen->auth_stack, "auth-stack");
    gtk_box_pack_start(GTK_BOX(center_container), screen->auth_stack, FALSE, FALSE, 0);

    GtkWidget *login_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(login_page, TRUE);
    gtk_widget_set_vexpand(login_page, TRUE);

    GtkWidget *login_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    add_class(login_card, "panel-card");
    gtk_widget_set_margin_top(login_card, 12);
    gtk_widget_set_margin_bottom(login_card, 12);
    gtk_box_pack_start(GTK_BOX(login_page), login_card, TRUE, TRUE, 0);

    GtkWidget *form_title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(form_title), "<span weight='900' size='28000' foreground='#FAF7F2'>Bon retour</span>");
    gtk_label_set_xalign(GTK_LABEL(form_title), 0.0f);
    gtk_widget_set_margin_bottom(form_title, 12);
    gtk_box_pack_start(GTK_BOX(login_card), form_title, FALSE, FALSE, 0);

    GtkWidget *form_subtitle = gtk_label_new("Accedez a votre espace de preparation aux entretiens");
    gtk_label_set_xalign(GTK_LABEL(form_subtitle), 0.0f);
    gtk_label_set_line_wrap(GTK_LABEL(form_subtitle), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(form_subtitle), 36);
    add_class(form_subtitle, "login-subtitle");
    gtk_widget_set_margin_bottom(form_subtitle, 28);
    gtk_box_pack_start(GTK_BOX(login_card), form_subtitle, FALSE, FALSE, 0);

    GtkWidget *form = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(form), 16);
    gtk_grid_set_column_spacing(GTK_GRID(form), 0);
    gtk_widget_set_hexpand(form, TRUE);
    gtk_box_pack_start(GTK_BOX(login_card), form, FALSE, FALSE, 0);

    GtkWidget *email_label = gtk_label_new("GMAIL");
    gtk_widget_set_halign(email_label, GTK_ALIGN_START);
    gtk_widget_set_margin_bottom(email_label, 8);
    add_class(email_label, "field-label");
    gtk_grid_attach(GTK_GRID(form), email_label, 0, 0, 1, 1);

    screen->username_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(screen->username_entry), "Votre adresse Gmail");
    add_class(screen->username_entry, "login-entry");
    gtk_widget_set_hexpand(screen->username_entry, TRUE);
    gtk_widget_set_size_request(screen->username_entry, -1, 52);
    gtk_grid_attach(GTK_GRID(form), screen->username_entry, 0, 1, 1, 1);

    GtkWidget *pass_label = gtk_label_new("PASSWORD");
    gtk_widget_set_halign(pass_label, GTK_ALIGN_START);
    gtk_widget_set_margin_top(pass_label, 8);
    gtk_widget_set_margin_bottom(pass_label, 8);
    add_class(pass_label, "field-label");
    gtk_grid_attach(GTK_GRID(form), pass_label, 0, 2, 1, 1);

    screen->password_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(screen->password_entry), "Votre mot de passe");
    gtk_entry_set_visibility(GTK_ENTRY(screen->password_entry), FALSE);
    gtk_entry_set_invisible_char(GTK_ENTRY(screen->password_entry), 0x2022);
    add_class(screen->password_entry, "login-entry");
    gtk_widget_set_hexpand(screen->password_entry, TRUE);
    gtk_widget_set_size_request(screen->password_entry, -1, 52);
    gtk_grid_attach(GTK_GRID(form), screen->password_entry, 0, 3, 1, 1);

    GtkWidget *options_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_margin_top(options_row, 16);
    gtk_widget_set_margin_bottom(options_row, 24);
    gtk_box_pack_start(GTK_BOX(login_card), options_row, FALSE, FALSE, 0);

    GtkWidget *remember_check = gtk_check_button_new_with_label("Se souvenir de moi");
    add_class(remember_check, "remember-check");
    gtk_box_pack_start(GTK_BOX(options_row), remember_check, FALSE, FALSE, 0);

    GtkWidget *forgot_btn = gtk_button_new_with_label("Mot de passe oublie ?");
    gtk_widget_set_halign(forgot_btn, GTK_ALIGN_END);
    add_class(forgot_btn, "forgot-button");
    gtk_box_pack_end(GTK_BOX(options_row), forgot_btn, FALSE, FALSE, 0);
    connect_interactive_button(forgot_btn);
    g_signal_connect(forgot_btn, "clicked", G_CALLBACK(on_open_recovery_clicked), screen);

    GtkWidget *login_feedback_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_bottom(login_feedback_area, 16);
    gtk_box_pack_start(GTK_BOX(login_card), login_feedback_area, FALSE, FALSE, 0);

    screen->login_status_label = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(screen->login_status_label), 0.0f);
    gtk_label_set_line_wrap(GTK_LABEL(screen->login_status_label), TRUE);
    add_class(screen->login_status_label, "feedback-label");
    gtk_box_pack_start(GTK_BOX(login_feedback_area), screen->login_status_label, FALSE, FALSE, 0);

    screen->login_progress_bar = gtk_progress_bar_new();
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(screen->login_progress_bar), TRUE);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(screen->login_progress_bar), 0.0);
    gtk_box_pack_start(GTK_BOX(login_feedback_area), screen->login_progress_bar, FALSE, FALSE, 0);

    GtkWidget *button_row = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_hexpand(button_row, TRUE);
    gtk_box_pack_start(GTK_BOX(login_card), button_row, FALSE, FALSE, 0);

    screen->login_button = gtk_button_new_with_label("Acceder a AURA");
    add_class(screen->login_button, "login-button");
    gtk_widget_set_hexpand(screen->login_button, TRUE);
    gtk_widget_set_size_request(screen->login_button, -1, 56);
    gtk_box_pack_start(GTK_BOX(button_row), screen->login_button, FALSE, FALSE, 0);
    g_signal_connect(screen->login_button, "clicked", G_CALLBACK(on_login_clicked), screen);
    connect_interactive_button(screen->login_button);

    GtkWidget *divider = gtk_label_new("────── OU ──────");
    add_class(divider, "divider-label");
    gtk_widget_set_margin_top(divider, 8);
    gtk_widget_set_margin_bottom(divider, 8);
    gtk_box_pack_start(GTK_BOX(button_row), divider, FALSE, FALSE, 0);

    GtkWidget *signup_button = gtk_button_new_with_label("Creer un compte");
    add_class(signup_button, "signup-button");
    gtk_widget_set_hexpand(signup_button, TRUE);
    gtk_widget_set_size_request(signup_button, -1, 56);
    gtk_box_pack_start(GTK_BOX(button_row), signup_button, FALSE, FALSE, 0);
    g_signal_connect(signup_button, "clicked", G_CALLBACK(on_open_signup_clicked), screen);
    connect_interactive_button(signup_button);

    screen->exit_button = gtk_button_new_with_label("Quitter");
    add_class(screen->exit_button, "exit-button");
    gtk_widget_set_hexpand(screen->exit_button, TRUE);
    gtk_widget_set_size_request(screen->exit_button, -1, 46);
    gtk_box_pack_start(GTK_BOX(button_row), screen->exit_button, FALSE, FALSE, 0);
    g_signal_connect(screen->exit_button, "clicked", G_CALLBACK(on_exit_clicked), screen);
    connect_interactive_button(screen->exit_button);

    gtk_stack_add_titled(GTK_STACK(screen->auth_stack), login_page, "login", "Connexion");
    screen->login_page = login_page;

    GtkWidget *signup_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(signup_page, TRUE);
    gtk_widget_set_vexpand(signup_page, TRUE);

    GtkWidget *signup_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    add_class(signup_card, "panel-card");
    gtk_widget_set_margin_top(signup_card, 12);
    gtk_widget_set_margin_bottom(signup_card, 12);
    gtk_box_pack_start(GTK_BOX(signup_page), signup_card, TRUE, TRUE, 0);

    GtkWidget *signup_title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(signup_title), "<span weight='900' size='28000' foreground='#FAF7F2'>Creer un compte</span>");
    gtk_label_set_xalign(GTK_LABEL(signup_title), 0.0f);
    gtk_widget_set_margin_bottom(signup_title, 12);
    gtk_box_pack_start(GTK_BOX(signup_card), signup_title, FALSE, FALSE, 0);

    GtkWidget *signup_subtitle = gtk_label_new("Rejoignez le simulateur d'entretiens ESISA");
    gtk_label_set_xalign(GTK_LABEL(signup_subtitle), 0.0f);
    gtk_label_set_line_wrap(GTK_LABEL(signup_subtitle), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(signup_subtitle), 40);
    add_class(signup_subtitle, "login-subtitle");
    gtk_widget_set_margin_bottom(signup_subtitle, 28);
    gtk_box_pack_start(GTK_BOX(signup_card), signup_subtitle, FALSE, FALSE, 0);

    GtkWidget *signup_form = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(signup_form), 14);
    gtk_grid_set_column_spacing(GTK_GRID(signup_form), 0);
    gtk_widget_set_hexpand(signup_form, TRUE);
    gtk_box_pack_start(GTK_BOX(signup_card), signup_form, FALSE, FALSE, 0);

    GtkWidget *fullname_label = gtk_label_new("NOM COMPLET");
    gtk_widget_set_halign(fullname_label, GTK_ALIGN_START);
    add_class(fullname_label, "field-label");
    gtk_grid_attach(GTK_GRID(signup_form), fullname_label, 0, 0, 1, 1);

    screen->signup_fullname_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(screen->signup_fullname_entry), "Votre nom complet");
    add_class(screen->signup_fullname_entry, "login-entry");
    gtk_widget_set_size_request(screen->signup_fullname_entry, -1, 50);
    gtk_grid_attach(GTK_GRID(signup_form), screen->signup_fullname_entry, 0, 1, 1, 1);

    GtkWidget *signup_email_label = gtk_label_new("GMAIL");
    gtk_widget_set_halign(signup_email_label, GTK_ALIGN_START);
    gtk_widget_set_margin_top(signup_email_label, 6);
    add_class(signup_email_label, "field-label");
    gtk_grid_attach(GTK_GRID(signup_form), signup_email_label, 0, 2, 1, 1);

    screen->signup_email_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(screen->signup_email_entry), "Votre adresse e-mail");
    add_class(screen->signup_email_entry, "login-entry");
    gtk_widget_set_size_request(screen->signup_email_entry, -1, 50);
    gtk_grid_attach(GTK_GRID(signup_form), screen->signup_email_entry, 0, 3, 1, 1);

    GtkWidget *signup_pass_label = gtk_label_new("PASSWORD");
    gtk_widget_set_halign(signup_pass_label, GTK_ALIGN_START);
    gtk_widget_set_margin_top(signup_pass_label, 6);
    add_class(signup_pass_label, "field-label");
    gtk_grid_attach(GTK_GRID(signup_form), signup_pass_label, 0, 4, 1, 1);

    screen->signup_password_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(screen->signup_password_entry), "Choisissez un mot de passe");
    gtk_entry_set_visibility(GTK_ENTRY(screen->signup_password_entry), FALSE);
    gtk_entry_set_invisible_char(GTK_ENTRY(screen->signup_password_entry), 0x2022);
    add_class(screen->signup_password_entry, "login-entry");
    gtk_widget_set_size_request(screen->signup_password_entry, -1, 50);
    gtk_grid_attach(GTK_GRID(signup_form), screen->signup_password_entry, 0, 5, 1, 1);

    GtkWidget *signup_confirm_label = gtk_label_new("CONFIRMER");
    gtk_widget_set_halign(signup_confirm_label, GTK_ALIGN_START);
    gtk_widget_set_margin_top(signup_confirm_label, 6);
    add_class(signup_confirm_label, "field-label");
    gtk_grid_attach(GTK_GRID(signup_form), signup_confirm_label, 0, 6, 1, 1);

    screen->signup_confirm_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(screen->signup_confirm_entry), "Confirmez le mot de passe");
    gtk_entry_set_visibility(GTK_ENTRY(screen->signup_confirm_entry), FALSE);
    gtk_entry_set_invisible_char(GTK_ENTRY(screen->signup_confirm_entry), 0x2022);
    add_class(screen->signup_confirm_entry, "login-entry");
    gtk_widget_set_size_request(screen->signup_confirm_entry, -1, 50);
    gtk_grid_attach(GTK_GRID(signup_form), screen->signup_confirm_entry, 0, 7, 1, 1);

    GtkWidget *signup_feedback_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_top(signup_feedback_area, 18);
    gtk_widget_set_margin_bottom(signup_feedback_area, 18);
    gtk_box_pack_start(GTK_BOX(signup_card), signup_feedback_area, FALSE, FALSE, 0);

    screen->signup_status_label = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(screen->signup_status_label), 0.0f);
    gtk_label_set_line_wrap(GTK_LABEL(screen->signup_status_label), TRUE);
    add_class(screen->signup_status_label, "feedback-label");
    gtk_box_pack_start(GTK_BOX(signup_feedback_area), screen->signup_status_label, FALSE, FALSE, 0);

    screen->signup_progress_bar = gtk_progress_bar_new();
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(screen->signup_progress_bar), TRUE);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(screen->signup_progress_bar), 0.0);
    gtk_box_pack_start(GTK_BOX(signup_feedback_area), screen->signup_progress_bar, FALSE, FALSE, 0);

    GtkWidget *signup_actions = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_box_pack_start(GTK_BOX(signup_card), signup_actions, FALSE, FALSE, 0);

    GtkWidget *create_button = gtk_button_new_with_label("Creer le compte");
    add_class(create_button, "login-button");
    gtk_widget_set_hexpand(create_button, TRUE);
    gtk_widget_set_size_request(create_button, -1, 56);
    gtk_box_pack_start(GTK_BOX(signup_actions), create_button, FALSE, FALSE, 0);
    g_signal_connect(create_button, "clicked", G_CALLBACK(on_create_account_clicked), screen);
    connect_interactive_button(create_button);

    screen->return_login_button = gtk_button_new_with_label("Retour connexion");
    add_class(screen->return_login_button, "return-button");
    gtk_widget_set_hexpand(screen->return_login_button, TRUE);
    gtk_widget_set_size_request(screen->return_login_button, -1, 50);
    gtk_box_pack_start(GTK_BOX(signup_actions), screen->return_login_button, FALSE, FALSE, 0);
    g_signal_connect(screen->return_login_button, "clicked", G_CALLBACK(on_return_login_clicked), screen);
    connect_interactive_button(screen->return_login_button);

    gtk_stack_add_titled(GTK_STACK(screen->auth_stack), signup_page, "signup", "Inscription");
    screen->signup_page = signup_page;

    GtkWidget *recovery_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(recovery_page, TRUE);
    gtk_widget_set_vexpand(recovery_page, TRUE);

    GtkWidget *recovery_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    add_class(recovery_card, "panel-card");
    gtk_widget_set_margin_top(recovery_card, 12);
    gtk_widget_set_margin_bottom(recovery_card, 12);
    gtk_box_pack_start(GTK_BOX(recovery_page), recovery_card, TRUE, TRUE, 0);

    GtkWidget *recovery_title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(recovery_title), "<span weight='900' size='28000' foreground='#FAF7F2'>Mot de passe oublie</span>");
    gtk_label_set_xalign(GTK_LABEL(recovery_title), 0.0f);
    gtk_widget_set_margin_bottom(recovery_title, 12);
    gtk_box_pack_start(GTK_BOX(recovery_card), recovery_title, FALSE, FALSE, 0);

    GtkWidget *recovery_copy = gtk_label_new("La recuperation de mot de passe n'est pas disponible en mode hors-ligne.");
    gtk_label_set_xalign(GTK_LABEL(recovery_copy), 0.0f);
    gtk_label_set_line_wrap(GTK_LABEL(recovery_copy), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(recovery_copy), 40);
    add_class(recovery_copy, "login-subtitle");
    gtk_widget_set_margin_bottom(recovery_copy, 24);
    gtk_box_pack_start(GTK_BOX(recovery_card), recovery_copy, FALSE, FALSE, 0);

    screen->recovery_status_label = gtk_label_new("Utilisez le bouton retour pour revenir a la connexion.");
    gtk_label_set_xalign(GTK_LABEL(screen->recovery_status_label), 0.0f);
    gtk_label_set_line_wrap(GTK_LABEL(screen->recovery_status_label), TRUE);
    add_class(screen->recovery_status_label, "feedback-label");
    gtk_widget_set_margin_bottom(screen->recovery_status_label, 20);
    gtk_box_pack_start(GTK_BOX(recovery_card), screen->recovery_status_label, FALSE, FALSE, 0);

    GtkWidget *recovery_actions = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_box_pack_start(GTK_BOX(recovery_card), recovery_actions, FALSE, FALSE, 0);

    screen->recovery_back_button = gtk_button_new_with_label("Retour connexion");
    add_class(screen->recovery_back_button, "return-button");
    gtk_widget_set_hexpand(screen->recovery_back_button, TRUE);
    gtk_widget_set_size_request(screen->recovery_back_button, -1, 50);
    gtk_box_pack_start(GTK_BOX(recovery_actions), screen->recovery_back_button, FALSE, FALSE, 0);
    g_signal_connect(screen->recovery_back_button, "clicked", G_CALLBACK(on_return_login_clicked), screen);
    connect_interactive_button(screen->recovery_back_button);

    gtk_stack_add_titled(GTK_STACK(screen->auth_stack), recovery_page, "recovery", "Recuperation");
    screen->recovery_page = recovery_page;

    /* ===== VERIFICATION PAGE ===== */
    GtkWidget *verification_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(verification_page, TRUE);
    gtk_widget_set_vexpand(verification_page, TRUE);

    GtkWidget *verification_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    add_class(verification_card, "panel-card");
    gtk_widget_set_margin_top(verification_card, 12);
    gtk_widget_set_margin_bottom(verification_card, 12);
    gtk_box_pack_start(GTK_BOX(verification_page), verification_card, TRUE, TRUE, 0);

    /* Title */
    GtkWidget *verification_title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(verification_title), "<span weight='900' size='28000' foreground='#FAF7F2'>Verification email</span>");
    gtk_label_set_xalign(GTK_LABEL(verification_title), 0.0f);
    gtk_widget_set_margin_bottom(verification_title, 12);
    gtk_box_pack_start(GTK_BOX(verification_card), verification_title, FALSE, FALSE, 0);

    /* Email display */
    GtkWidget *verify_email_intro = gtk_label_new("Code de verification envoye a :");
    gtk_label_set_xalign(GTK_LABEL(verify_email_intro), 0.0f);
    add_class(verify_email_intro, "login-subtitle");
    gtk_widget_set_margin_bottom(verify_email_intro, 6);
    gtk_box_pack_start(GTK_BOX(verification_card), verify_email_intro, FALSE, FALSE, 0);

    screen->verification_email_label = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(screen->verification_email_label), 0.0f);
    gtk_label_set_markup(GTK_LABEL(screen->verification_email_label), "<span foreground='#E8A838' weight='700' size='13000'></span>");
    gtk_widget_set_margin_bottom(screen->verification_email_label, 20);
    gtk_box_pack_start(GTK_BOX(verification_card), screen->verification_email_label, FALSE, FALSE, 0);

    /* Code input */
    GtkWidget *code_label = gtk_label_new("CODE DE VERIFICATION");
    gtk_label_set_xalign(GTK_LABEL(code_label), 0.0f);
    add_class(code_label, "field-label");
    gtk_widget_set_margin_bottom(code_label, 8);
    gtk_box_pack_start(GTK_BOX(verification_card), code_label, FALSE, FALSE, 0);

    screen->verification_code_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(screen->verification_code_entry), "Code a 6 chiffres");
    add_class(screen->verification_code_entry, "name-entry");
    gtk_entry_set_max_length(GTK_ENTRY(screen->verification_code_entry), 6);
    gtk_widget_set_margin_bottom(screen->verification_code_entry, 20);
    gtk_box_pack_start(GTK_BOX(verification_card), screen->verification_code_entry, FALSE, FALSE, 0);

    /* Status label */
    screen->verification_status_label = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(screen->verification_status_label), 0.0f);
    gtk_label_set_line_wrap(GTK_LABEL(screen->verification_status_label), TRUE);
    add_class(screen->verification_status_label, "feedback-label");
    gtk_widget_set_margin_bottom(screen->verification_status_label, 20);
    gtk_box_pack_start(GTK_BOX(verification_card), screen->verification_status_label, FALSE, FALSE, 0);

    /* Action buttons */
    GtkWidget *verification_actions = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_box_pack_start(GTK_BOX(verification_card), verification_actions, FALSE, FALSE, 0);

    screen->verification_confirm_button = gtk_button_new_with_label("Verifier le code");
    add_class(screen->verification_confirm_button, "primary-action");
    gtk_widget_set_hexpand(screen->verification_confirm_button, TRUE);
    gtk_widget_set_size_request(screen->verification_confirm_button, -1, 50);
    gtk_box_pack_start(GTK_BOX(verification_actions), screen->verification_confirm_button, FALSE, FALSE, 0);
    g_signal_connect(screen->verification_confirm_button, "clicked", G_CALLBACK(on_verify_code_clicked), screen);
    connect_interactive_button(screen->verification_confirm_button);

    /* Resend code section */
    GtkWidget *resend_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_box_pack_start(GTK_BOX(verification_actions), resend_container, FALSE, FALSE, 0);

    screen->verification_resend_label = gtk_label_new("Vous n'avez pas recu le code ?");
    gtk_label_set_xalign(GTK_LABEL(screen->verification_resend_label), 0.5f);
    add_class(screen->verification_resend_label, "login-subtitle");
    gtk_widget_set_margin_bottom(screen->verification_resend_label, 4);
    gtk_box_pack_start(GTK_BOX(resend_container), screen->verification_resend_label, FALSE, FALSE, 0);

    screen->verification_resend_button = gtk_button_new_with_label("Resend Code (wait 10s)");
    add_class(screen->verification_resend_button, "secondary-action");
    gtk_widget_set_hexpand(screen->verification_resend_button, TRUE);
    gtk_widget_set_size_request(screen->verification_resend_button, -1, 45);
    gtk_widget_set_sensitive(screen->verification_resend_button, FALSE);
    gtk_box_pack_start(GTK_BOX(resend_container), screen->verification_resend_button, FALSE, FALSE, 0);
    g_signal_connect(screen->verification_resend_button, "clicked", G_CALLBACK(on_verification_resend_clicked), screen);
    connect_interactive_button(screen->verification_resend_button);

    screen->verification_back_button = gtk_button_new_with_label("Retour connexion");
    add_class(screen->verification_back_button, "return-button");
    gtk_widget_set_hexpand(screen->verification_back_button, TRUE);
    gtk_widget_set_size_request(screen->verification_back_button, -1, 50);
    gtk_box_pack_start(GTK_BOX(verification_actions), screen->verification_back_button, FALSE, FALSE, 0);
    g_signal_connect(screen->verification_back_button, "clicked", G_CALLBACK(on_verification_back_clicked), screen);
    connect_interactive_button(screen->verification_back_button);

    gtk_stack_add_titled(GTK_STACK(screen->auth_stack), verification_page, "verification", "Verification");
    screen->verification_page = verification_page;

    gtk_stack_set_transition_type(GTK_STACK(screen->auth_stack), GTK_STACK_TRANSITION_TYPE_CROSSFADE);
    gtk_stack_set_visible_child_name(GTK_STACK(screen->auth_stack), "login");

    // Store panel reference for animations
    screen->panel = center_container;
    screen->panel_start_margin_top = 60;
    screen->panel_end_margin_top = 60;

    screen->background_tick_id = g_timeout_add(16, animation_tick_cb, screen);
    screen->typing_tick_id = 0; /* will start when typing_label is shown */

    /* create a top overlay cover area to animate fade */
    screen->cover_area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(screen->cover_area, TRUE);
    gtk_widget_set_vexpand(screen->cover_area, TRUE);
    g_signal_connect(screen->cover_area, "draw", G_CALLBACK(cover_draw_cb), screen);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), screen->cover_area);
    g_object_add_weak_pointer(G_OBJECT(screen->cover_area), (gpointer *)&screen->cover_area);
    screen->overlay_alpha = 1.0;
    screen->entry_anim_progress = 0;
    screen->components_shown = FALSE;

    screen->auth_messages = g_ptr_array_new_with_free_func(g_free);
    g_ptr_array_add(screen->auth_messages, g_strdup("Verification des identifiants..."));
    g_ptr_array_add(screen->auth_messages, g_strdup("Acces autorise..."));
    g_ptr_array_add(screen->auth_messages, g_strdup("Chargement de l'espace AURA..."));
    g_ptr_array_add(screen->auth_messages, g_strdup("Initialisation de l'environnement..."));

    gtk_widget_hide(screen->auth_stack);

    fprintf(dbg, "[LOGIN] Before gtk_widget_show_all\n");
    fflush(dbg);
    gtk_widget_show_all(screen->window);
    gtk_window_present(GTK_WINDOW(screen->window));

    fprintf(dbg, "[LOGIN] After gtk_widget_show_all\n");
    fflush(dbg);

    if (screen->auth_stack != NULL) {
        gtk_widget_hide(screen->auth_stack);
    }

    fprintf(dbg, "[LOGIN] Before entry animation start\n");
    fflush(dbg);

    /* start entry animation after window shown */
    if (screen->entry_anim_id == 0) {
        screen->entry_anim_id = g_timeout_add(16, entry_animation_tick_cb, screen);
    }

    fprintf(dbg, "[LOGIN] Before g_main_loop_run\n");
    fflush(dbg);

    g_main_loop_run(screen->loop);

    fprintf(dbg, "[LOGIN] After g_main_loop_run, accepted=%d\n", screen->accepted);
    fflush(dbg);

    gboolean accepted = screen->accepted;
    destroy_login_screen(screen);
    fclose(dbg);
    return accepted;
}