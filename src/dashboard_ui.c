#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "dashboard_ui.h"
#include "questions.h"
#include "question_bank.h"
#include "interview.h"
#include "evaluation.h"
#include "report.h"
#include "session.h"
#include "db.h"
#include "stats.h"
#include "filesystem.h"
#include "config.h"

/* Minimal, self-contained AI Dashboard UI
 * - Left navigation sidebar
 * - Animated stats cards
 * - User information panel
 * - AI assistant status
 * - Progress overview and session counters
 * - Dynamic welcome message
 * Styles: futuristic dark, neon cyan, glassmorphism
 */

typedef struct {
    GtkWidget *window;
    GtkWidget *sidebar;
    GtkWidget *stack;
    GtkWidget *welcome_label;
    GtkWidget *ai_status_label;
    GtkWidget *progress_bar;
    GtkWidget *session_counter_label;
    GtkWidget *stat_cards[4];
    GtkWidget *nav_buttons[5];
    gboolean relaunch_login;
    gboolean close_requested;
    gboolean interactions_enabled;
    guint tick_id;
    guint elapsed_seconds;
    guint interaction_enable_id;
    gchar *selected_level;
    gchar *selected_domain;
    gchar *lb_domain_filter;
    gchar *lb_level_filter;
    gchar *username;
    GtkWidget *reports_container;
    GtkWidget *leaderboard_container;
    GtkWidget *profile_container;
    GtkWidget *settings_container;
} AuraDashboard;

static gboolean g_dashboard_restart_requested = FALSE;

gboolean aura_dashboard_restart_requested(void) {
    return g_dashboard_restart_requested;
}

void aura_dashboard_clear_restart_request(void) {
    g_dashboard_restart_requested = FALSE;
}

static gboolean dashboard_button_enter_cb(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data) {
    (void)event;
    (void)user_data;
    if (widget != NULL) {
        GdkDisplay *display = gtk_widget_get_display(widget);
        if (display != NULL) {
            gdk_display_beep(display);
            if (gtk_widget_get_realized(widget)) {
                GdkWindow *gdk_window = gtk_widget_get_window(widget);
                if (gdk_window != NULL) {
                    GdkCursor *cursor = gdk_cursor_new_from_name(display, "pointer");
                    if (cursor == NULL) {
                        cursor = gdk_cursor_new_for_display(display, GDK_HAND2);
                    }
                    if (cursor != NULL) {
                        gdk_window_set_cursor(gdk_window, cursor);
                        g_object_unref(cursor);
                    }
                }
            }
        }
    }
    return FALSE;
}

static gboolean dashboard_button_leave_cb(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data) {
    (void)event;
    (void)user_data;
    if (widget != NULL && gtk_widget_get_realized(widget)) {
        GdkWindow *gdk_window = gtk_widget_get_window(widget);
        if (gdk_window != NULL) {
            gdk_window_set_cursor(gdk_window, NULL);
        }
    }
    return FALSE;
}

static void connect_dashboard_button(GtkWidget *widget) {
    if (widget == NULL) {
        return;
    }

    gtk_widget_add_events(widget, GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
    g_signal_connect(widget, "enter-notify-event", G_CALLBACK(dashboard_button_enter_cb), NULL);
    g_signal_connect(widget, "leave-notify-event", G_CALLBACK(dashboard_button_leave_cb), NULL);
}

static void add_class(GtkWidget *widget, const char *class_name);
static void style_flat_button(GtkWidget *btn);
static void domain_dialog_show(AuraDashboard *dash);
static void level_dialog_show(AuraDashboard *dash);
static void interview_update_progress(GtkWidget *dlg, Interview *iv);
static void interview_update_feedback(GtkWidget *dlg, Interview *iv, int idx);
static void interview_evaluate_current_answer(GtkWidget *dlg, Interview *iv);
static void refresh_leaderboard_page(AuraDashboard *dash);
static void on_lb_filter_clicked(GtkWidget *widget, gpointer user_data);
static void on_report_pdf_open(GtkWidget *widget, gpointer user_data);

static void style_flat_button(GtkWidget *btn) {
    if (btn && GTK_IS_BUTTON(btn)) {
        gtk_button_set_relief(GTK_BUTTON(btn), GTK_RELIEF_NONE);
    }
}

static gboolean enable_dashboard_interactions_cb(gpointer user_data) {
    AuraDashboard *dash = (AuraDashboard *)user_data;
    if (dash != NULL) {
        dash->interactions_enabled = TRUE;
        dash->interaction_enable_id = 0;
    }
    return G_SOURCE_REMOVE;
}

static void on_dashboard_destroy(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AuraDashboard *dash = (AuraDashboard *)user_data;

    if (dash == NULL) {
        gtk_main_quit();
        return;
    }

    if (dash->tick_id != 0) {
        g_source_remove(dash->tick_id);
        dash->tick_id = 0;
    }
    if (dash->interaction_enable_id != 0) {
        g_source_remove(dash->interaction_enable_id);
        dash->interaction_enable_id = 0;
    }

    FILE *dbg = fopen("bin/login_screen_debug.log", "a");
    if (dbg != NULL) {
        fprintf(dbg, "[DASHBOARD] destroy event (requested=%d)\n", dash->close_requested ? 1 : 0);
        fclose(dbg);
    }

    if (!dash->close_requested) {
        g_dashboard_restart_requested = TRUE;
        dash->relaunch_login = FALSE;
    } else {
        g_dashboard_restart_requested = FALSE;
    }

    gtk_main_quit();
}

static GtkWidget *create_dashboard_page(const gchar *title, const gchar *copy, GtkWidget **body_out) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(page, TRUE);
    gtk_widget_set_vexpand(page, TRUE);

    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    add_class(card, "glass-card");
    gtk_widget_set_margin_top(card, 20);
    gtk_widget_set_margin_bottom(card, 20);
    gtk_widget_set_margin_start(card, 20);
    gtk_widget_set_margin_end(card, 20);
    gtk_box_pack_start(GTK_BOX(page), card, TRUE, TRUE, 0);

    GtkWidget *title_label = gtk_label_new(title);
    gtk_label_set_xalign(GTK_LABEL(title_label), 0.0f);
    add_class(title_label, "panel-heading");
    gtk_box_pack_start(GTK_BOX(card), title_label, FALSE, FALSE, 0);

    GtkWidget *copy_label = gtk_label_new(copy);
    gtk_label_set_xalign(GTK_LABEL(copy_label), 0.0f);
    gtk_label_set_line_wrap(GTK_LABEL(copy_label), TRUE);
    add_class(copy_label, "panel-copy");
    gtk_box_pack_start(GTK_BOX(card), copy_label, FALSE, FALSE, 0);

    if (body_out) {
        GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
        gtk_widget_set_vexpand(scroll, TRUE);
        GtkWidget *body = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_container_add(GTK_CONTAINER(scroll), body);
        gtk_box_pack_start(GTK_BOX(card), scroll, TRUE, TRUE, 8);
        *body_out = body;
    }

    return page;
}

static void set_active_nav(AuraDashboard *dash, GtkWidget *active_button) {
    if (dash == NULL) {
        return;
    }

    for (guint i = 0; i < G_N_ELEMENTS(dash->nav_buttons); i++) {
        if (dash->nav_buttons[i] != NULL) {
            gtk_style_context_remove_class(gtk_widget_get_style_context(dash->nav_buttons[i]), "active");
        }
    }

    if (active_button != NULL) {
        gtk_style_context_add_class(gtk_widget_get_style_context(active_button), "active");
    }
}

static void switch_dashboard_page(AuraDashboard *dash, const gchar *page_name, GtkWidget *button) {
    if (dash == NULL || dash->stack == NULL || page_name == NULL) {
        return;
    }

    gtk_stack_set_visible_child_name(GTK_STACK(dash->stack), page_name);
    set_active_nav(dash, button);
}

static void clear_container_children(GtkWidget *container) {
    if (!container) return;
    GList *children = gtk_container_get_children(GTK_CONTAINER(container));
    for (GList *l = children; l; l = l->next) gtk_widget_destroy(GTK_WIDGET(l->data));
    g_list_free(children);
}

static void on_report_pdf_open(GtkWidget *widget, gpointer user_data) {
    (void)user_data;
    const char *path = (const char *)g_object_get_data(G_OBJECT(widget), "pdf_path");
    if (path) report_open_file(path);
}

static void refresh_reports_page(AuraDashboard *dash) {
    if (!dash || !dash->reports_container) return;
    clear_container_children(dash->reports_container);

    GtkWidget *hist_title = gtk_label_new("Historique SQLite");
    gtk_label_set_xalign(GTK_LABEL(hist_title), 0.0f);
    add_class(hist_title, "dash-accent");
    gtk_box_pack_start(GTK_BOX(dash->reports_container), hist_title, FALSE, FALSE, 8);

    DbScoreRow rows[20];
    int n = db_fetch_recent_scores(rows, 20, dash->username);
    if (n == 0) {
        GtkWidget *empty = gtk_label_new("Aucun entretien enregistre pour le moment.");
        add_class(empty, "panel-copy");
        gtk_box_pack_start(GTK_BOX(dash->reports_container), empty, FALSE, FALSE, 0);
    } else {
        for (int i = 0; i < n; i++) {
            char line[256];
            snprintf(line, sizeof(line), "%s — Score: %d/10 — %s",
                rows[i].user_name, rows[i].score, rows[i].categorie);
            GtkWidget *lbl = gtk_label_new(line);
            gtk_label_set_xalign(GTK_LABEL(lbl), 0.0f);
            add_class(lbl, "panel-copy");
            gtk_box_pack_start(GTK_BOX(dash->reports_container), lbl, FALSE, FALSE, 4);
        }
    }

    GtkWidget *pdf_title = gtk_label_new("Rapports PDF (par session)");
    gtk_label_set_xalign(GTK_LABEL(pdf_title), 0.0f);
    add_class(pdf_title, "dash-accent");
    gtk_widget_set_margin_top(pdf_title, 16);
    gtk_box_pack_start(GTK_BOX(dash->reports_container), pdf_title, FALSE, FALSE, 8);

    char pdf_paths[10][1024];
    int pn = report_list_pdfs(pdf_paths, 10);
    if (pn == 0) {
        GtkWidget *no_pdf = gtk_label_new("Aucun PDF — terminez un entretien pour generer un rapport.");
        add_class(no_pdf, "panel-copy");
        gtk_box_pack_start(GTK_BOX(dash->reports_container), no_pdf, FALSE, FALSE, 0);
        return;
    }
    for (int i = 0; i < pn; i++) {
        const char *slash = strrchr(pdf_paths[i], '/');
        if (!slash) slash = strrchr(pdf_paths[i], '\\');
        const char *name = slash ? slash + 1 : pdf_paths[i];
        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        GtkWidget *lbl = gtk_label_new(name);
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.0f);
        add_class(lbl, "panel-copy");
        gtk_box_pack_start(GTK_BOX(row), lbl, TRUE, TRUE, 0);
        GtkWidget *btn = gtk_button_new_with_label("Ouvrir PDF");
        style_flat_button(btn);
        add_class(btn, "secondary-action");
        g_object_set_data_full(G_OBJECT(btn), "pdf_path", g_strdup(pdf_paths[i]), g_free);
        g_signal_connect(btn, "clicked", G_CALLBACK(on_report_pdf_open), NULL);
        gtk_box_pack_end(GTK_BOX(row), btn, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(dash->reports_container), row, FALSE, FALSE, 4);
    }
}

static GtkWidget *make_filter_chip(AuraDashboard *dash, GtkWidget *parent,
    const char *label, const char *filter_kind, const char *filter_value) {
    GtkWidget *btn = gtk_button_new_with_label(label);
    style_flat_button(btn);
    add_class(btn, "secondary-action");
    g_object_set_data(G_OBJECT(btn), "dash", dash);
    g_object_set_data(G_OBJECT(btn), "filter_kind", (gpointer)filter_kind);
    g_object_set_data(G_OBJECT(btn), "filter_value", (gpointer)filter_value);
    g_signal_connect(btn, "clicked", G_CALLBACK(on_lb_filter_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(parent), btn, FALSE, FALSE, 4);
    return btn;
}

static void on_lb_filter_clicked(GtkWidget *widget, gpointer user_data) {
    (void)user_data;
    AuraDashboard *dash = (AuraDashboard *)g_object_get_data(G_OBJECT(widget), "dash");
    const char *kind = (const char *)g_object_get_data(G_OBJECT(widget), "filter_kind");
    const char *value = (const char *)g_object_get_data(G_OBJECT(widget), "filter_value");
    if (!dash || !kind) return;
    if (strcmp(kind, "domain") == 0) {
        g_free(dash->lb_domain_filter);
        dash->lb_domain_filter = (value && value[0]) ? g_strdup(value) : NULL;
    } else if (strcmp(kind, "level") == 0) {
        g_free(dash->lb_level_filter);
        dash->lb_level_filter = (value && value[0]) ? g_strdup(value) : NULL;
    }
    refresh_leaderboard_page(dash);
}

static void refresh_leaderboard_page(AuraDashboard *dash) {
    if (!dash || !dash->leaderboard_container) return;
    clear_container_children(dash->leaderboard_container);

    GtkWidget *filt_title = gtk_label_new("Filtres");
    gtk_label_set_xalign(GTK_LABEL(filt_title), 0.0f);
    add_class(filt_title, "dash-accent");
    gtk_box_pack_start(GTK_BOX(dash->leaderboard_container), filt_title, FALSE, FALSE, 4);

    GtkWidget *dom_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    make_filter_chip(dash, dom_row, "Toutes matieres", "domain", "");
    make_filter_chip(dash, dom_row, "Architecture", "domain", "Architecture");
    make_filter_chip(dash, dom_row, "Algorithmique", "domain", "Algorithmique");
    make_filter_chip(dash, dom_row, "Prog. C", "domain", "Programmation C");
    gtk_box_pack_start(GTK_BOX(dash->leaderboard_container), dom_row, FALSE, FALSE, 4);

    GtkWidget *lvl_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    make_filter_chip(dash, lvl_row, "Tous niveaux", "level", "");
    make_filter_chip(dash, lvl_row, "Junior", "level", "Junior");
    make_filter_chip(dash, lvl_row, "Intermediate", "level", "Intermediate");
    make_filter_chip(dash, lvl_row, "Advanced", "level", "Advanced");
    make_filter_chip(dash, lvl_row, "Expert", "level", "Expert");
    gtk_box_pack_start(GTK_BOX(dash->leaderboard_container), lvl_row, FALSE, FALSE, 8);

    char filt_info[256];
    snprintf(filt_info, sizeof(filt_info), "Actif: %s | %s",
        dash->lb_domain_filter && dash->lb_domain_filter[0] ? dash->lb_domain_filter : "Toutes matieres",
        dash->lb_level_filter && dash->lb_level_filter[0] ? dash->lb_level_filter : "Tous niveaux");
    GtkWidget *filt_lbl = gtk_label_new(filt_info);
    gtk_label_set_xalign(GTK_LABEL(filt_lbl), 0.0f);
    add_class(filt_lbl, "panel-copy");
    gtk_box_pack_start(GTK_BOX(dash->leaderboard_container), filt_lbl, FALSE, FALSE, 6);

    GtkWidget *hdr = gtk_label_new("Rang | Etudiant | Moyenne | Meilleur | Entretiens | Matiere");
    gtk_label_set_xalign(GTK_LABEL(hdr), 0.0f);
    add_class(hdr, "dash-accent");
    gtk_box_pack_start(GTK_BOX(dash->leaderboard_container), hdr, FALSE, FALSE, 8);

    DbLeaderboardRow rows[20];
    int n = db_fetch_leaderboard_pro(rows, 20,
        dash->lb_domain_filter, dash->lb_level_filter);

    if (n == 0) {
        GtkWidget *empty = gtk_label_new("Classement vide — passez un entretien pour apparaitre ici.");
        add_class(empty, "panel-copy");
        gtk_box_pack_start(GTK_BOX(dash->leaderboard_container), empty, FALSE, FALSE, 0);
        return;
    }
    for (int i = 0; i < n; i++) {
        char line[384];
        snprintf(line, sizeof(line), "#%-2d  %-18s  %3d/10  %3d/10  %3d sess.  %s",
            rows[i].rank, rows[i].user_name, rows[i].avg_score,
            rows[i].best_score, rows[i].interviews, rows[i].domain);
        GtkWidget *lbl = gtk_label_new(line);
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.0f);
        add_class(lbl, "panel-copy");
        if (i == 0) add_class(lbl, "leaderboard-gold");
        gtk_box_pack_start(GTK_BOX(dash->leaderboard_container), lbl, FALSE, FALSE, 4);
    }
}

static void refresh_profile_page(AuraDashboard *dash) {
    if (!dash || !dash->profile_container) return;
    clear_container_children(dash->profile_container);
    Statistics stats = stats_get_user_stats(dash->username);
    char buf[512];
    snprintf(buf, sizeof(buf), "Nom: %s", g_aura_session.fullname[0] ? g_aura_session.fullname : "—");
    gtk_box_pack_start(GTK_BOX(dash->profile_container), gtk_label_new(buf), FALSE, FALSE, 6);
    snprintf(buf, sizeof(buf), "Email: %s", dash->username ? dash->username : "—");
    gtk_box_pack_start(GTK_BOX(dash->profile_container), gtk_label_new(buf), FALSE, FALSE, 6);
    snprintf(buf, sizeof(buf), "Entretiens passes: %d", stats.interviews_taken);
    gtk_box_pack_start(GTK_BOX(dash->profile_container), gtk_label_new(buf), FALSE, FALSE, 6);
    snprintf(buf, sizeof(buf), "Score moyen: %.1f/10", stats.average_score);
    gtk_box_pack_start(GTK_BOX(dash->profile_container), gtk_label_new(buf), FALSE, FALSE, 6);
}

static void refresh_settings_page(AuraDashboard *dash) {
    if (!dash || !dash->settings_container) return;
    clear_container_children(dash->settings_container);
    const char *key = aura_config_get_string("groq_api_key", "");
    gboolean has_key = (key && key[0] != '\0') || (getenv("AURA_API_KEY") && getenv("AURA_API_KEY")[0]);
    GtkWidget *api = gtk_label_new(has_key ? "Cle API Groq: configuree" : "Cle API Groq: manquante (ajoutez groq_api_key dans config/aura.cfg)");
    gtk_box_pack_start(GTK_BOX(dash->settings_container), api, FALSE, FALSE, 6);
    GtkWidget *db = gtk_label_new("Base locale: SQLite (data/local.db)");
    gtk_box_pack_start(GTK_BOX(dash->settings_container), db, FALSE, FALSE, 6);
    GtkWidget *mat = gtk_label_new("Matieres: Architecture des ordinateurs, Algorithmique, Programmation C");
    gtk_box_pack_start(GTK_BOX(dash->settings_container), mat, FALSE, FALSE, 6);
    char qbuf[256];
    snprintf(qbuf, sizeof(qbuf),
        "Banque de questions: %d par niveau (Architecture), %d (Algorithmique), %d (Programmation C) — Junior",
        qb_count("Architecture des ordinateurs", "Junior"),
        qb_count("Algorithmique", "Junior"),
        qb_count("Programmation C", "Junior"));
    gtk_box_pack_start(GTK_BOX(dash->settings_container), gtk_label_new(qbuf), FALSE, FALSE, 6);
}

static void refresh_dashboard_metrics(AuraDashboard *dash) {
    if (!dash) return;
    Statistics stats = stats_get_user_stats(dash->username);
    char buf[64];
    snprintf(buf, sizeof(buf), "%d", stats.interviews_taken);
    if (dash->stat_cards[0]) {
        GtkWidget *w = gtk_bin_get_child(GTK_BIN(dash->stat_cards[0]));
        if (GTK_IS_LABEL(w)) gtk_label_set_text(GTK_LABEL(w), buf);
    }
    snprintf(buf, sizeof(buf), "%.0f", stats.average_score);
    if (dash->stat_cards[1]) {
        GtkWidget *w = gtk_bin_get_child(GTK_BIN(dash->stat_cards[1]));
        if (GTK_IS_LABEL(w)) gtk_label_set_text(GTK_LABEL(w), buf);
    }
    snprintf(buf, sizeof(buf), "%d", db_count_interviews(NULL));
    if (dash->stat_cards[2]) {
        GtkWidget *w = gtk_bin_get_child(GTK_BIN(dash->stat_cards[2]));
        if (GTK_IS_LABEL(w)) gtk_label_set_text(GTK_LABEL(w), buf);
    }
    snprintf(buf, sizeof(buf), "%d", dash->elapsed_seconds / 60);
    if (dash->stat_cards[3]) {
        GtkWidget *w = gtk_bin_get_child(GTK_BIN(dash->stat_cards[3]));
        if (GTK_IS_LABEL(w)) gtk_label_set_text(GTK_LABEL(w), buf);
    }
    if (dash->welcome_label) {
        char welcome[256];
        snprintf(welcome, sizeof(welcome), "Bienvenue %s — simulateur d'entretiens ESISA",
            g_aura_session.fullname[0] ? g_aura_session.fullname : dash->username);
        gtk_label_set_text(GTK_LABEL(dash->welcome_label), welcome);
    }
}

static void on_dashboard_nav_clicked(GtkWidget *widget, gpointer user_data) {
    AuraDashboard *dash = (AuraDashboard *)user_data;
    const gchar *route = g_object_get_data(G_OBJECT(widget), "route");

    FILE *dbg = fopen("bin/login_screen_debug.log", "a");
    if (dbg != NULL) {
        fprintf(dbg, "[DASHBOARD] nav clicked route=%s\n", route != NULL ? route : "(null)");
        fclose(dbg);
    }

    if (dash == NULL || route == NULL) {
        return;
    }

    if (!dash->interactions_enabled) {
        FILE *guard = fopen("bin/login_screen_debug.log", "a");
        if (guard != NULL) {
            fprintf(guard, "[DASHBOARD] ignoring early nav click route=%s\n", route);
            fclose(guard);
        }
        return;
    }

    if (g_strcmp0(route, "logout") == 0) {
        dash->relaunch_login = TRUE;
        dash->close_requested = TRUE;
        g_dashboard_restart_requested = FALSE;
        if (dash->window != NULL) {
            gtk_widget_destroy(dash->window);
        }
        return;
    }

    if (g_strcmp0(route, "exit") == 0) {
        dash->relaunch_login = FALSE;
        dash->close_requested = TRUE;
        g_dashboard_restart_requested = FALSE;
        if (dash->window != NULL) {
            gtk_widget_destroy(dash->window);
        }
        return;
    }

    if (g_strcmp0(route, "reports") == 0) refresh_reports_page(dash);
    else if (g_strcmp0(route, "leaderboard") == 0) refresh_leaderboard_page(dash);
    else if (g_strcmp0(route, "profile") == 0) refresh_profile_page(dash);
    else if (g_strcmp0(route, "settings") == 0) refresh_settings_page(dash);

    switch_dashboard_page(dash, route, widget);
}

/* Interview dialog: simple modal that shows random questions from questions.c */
static void on_interview_next_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    GtkWidget *label = GTK_WIDGET(user_data);
    // legacy: keep for compatibility
    Question *q = questions_get_random(NULL, NULL);
    if (q && q->text) gtk_label_set_text(GTK_LABEL(label), q->text);
    else gtk_label_set_text(GTK_LABEL(label), "(no question available)");
}

/* Interview dialog callbacks */
static void interview_dialog_update(GtkWidget *dlg, GtkWidget *q_label, GtkWidget *answer_view) {
    Interview *iv = (Interview *)g_object_get_data(G_OBJECT(dlg), "interview");
    if (!iv) return;
    char qbuf[4096];
    if (interview_get_current_question(iv, qbuf, sizeof(qbuf))) {
        gtk_label_set_text(GTK_LABEL(q_label), qbuf);
    }
    int idx = interview_get_current_index(iv);
    const char *existing = interview_get_answer(iv, idx);
    GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(answer_view));
    if (existing) gtk_text_buffer_set_text(tb, existing, -1);
    else gtk_text_buffer_set_text(tb, "", -1);
}

static void interview_dialog_prev_cb(GtkWidget *widget, gpointer user_data) {
    GtkWidget *dlg = gtk_widget_get_toplevel(widget);
    GtkWidget *q_label = GTK_WIDGET(g_object_get_data(G_OBJECT(dlg), "q_label"));
    GtkWidget *answer_view = GTK_WIDGET(g_object_get_data(G_OBJECT(dlg), "answer_view"));
    Interview *iv = (Interview *)g_object_get_data(G_OBJECT(dlg), "interview");
    if (!iv) return;
    // Save current answer
    int cur = interview_get_current_index(iv);
    GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(answer_view));
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(tb, &start);
    gtk_text_buffer_get_end_iter(tb, &end);
    gchar *txt = gtk_text_buffer_get_text(tb, &start, &end, FALSE);
    if (cur >= 0) interview_record_answer(iv, cur, txt);
    g_free(txt);
    if (cur >= 0) {
        interview_evaluate_current(iv, cur);
        interview_update_feedback(dlg, iv, cur);
    }
    char qbuf[4096];
    if (interview_prev(iv, qbuf, sizeof(qbuf))) {
        interview_dialog_update(dlg, q_label, answer_view);
        interview_update_progress(dlg, iv);
        int new_idx = interview_get_current_index(iv);
        interview_update_feedback(dlg, iv, new_idx);
    }
}

static void interview_dialog_validate_cb(GtkWidget *widget, gpointer user_data) {
    (void)user_data;
    GtkWidget *dlg = gtk_widget_get_toplevel(widget);
    Interview *iv = (Interview *)g_object_get_data(G_OBJECT(dlg), "interview");
    if (!iv) return;
    interview_evaluate_current_answer(dlg, iv);
    int cur = interview_get_current_index(iv);
    interview_update_feedback(dlg, iv, cur);
}

static void interview_dialog_next_cb(GtkWidget *widget, gpointer user_data) {
    GtkWidget *dlg = gtk_widget_get_toplevel(widget);
    GtkWidget *q_label = GTK_WIDGET(g_object_get_data(G_OBJECT(dlg), "q_label"));
    GtkWidget *answer_view = GTK_WIDGET(g_object_get_data(G_OBJECT(dlg), "answer_view"));
    Interview *iv = (Interview *)g_object_get_data(G_OBJECT(dlg), "interview");
    if (!iv) return;
    // Save current answer
    int cur = interview_get_current_index(iv);
    GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(answer_view));
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(tb, &start);
    gtk_text_buffer_get_end_iter(tb, &end);
    gchar *txt = gtk_text_buffer_get_text(tb, &start, &end, FALSE);
    if (cur >= 0) interview_record_answer(iv, cur, txt);
    g_free(txt);
    interview_evaluate_current_answer(dlg, iv);
    char qbuf[4096];
    if (interview_next(iv, qbuf, sizeof(qbuf))) {
        interview_dialog_update(dlg, q_label, answer_view);
        interview_update_progress(dlg, iv);
        int new_idx = interview_get_current_index(iv);
        interview_update_feedback(dlg, iv, new_idx);
    }
}

static void interview_dialog_finish_cb(GtkWidget *widget, gpointer user_data) {
    GtkWidget *dlg = gtk_widget_get_toplevel(widget);
    Interview *iv = (Interview *)g_object_get_data(G_OBJECT(dlg), "interview");
    if (!iv) {
        gtk_widget_destroy(dlg);
        return;
    }
    // Save current answer
    int cur = interview_get_current_index(iv);
    GtkWidget *answer_view = GTK_WIDGET(g_object_get_data(G_OBJECT(dlg), "answer_view"));
    if (answer_view && cur >= 0) {
        GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(answer_view));
        GtkTextIter start, end;
        gtk_text_buffer_get_start_iter(tb, &start);
        gtk_text_buffer_get_end_iter(tb, &end);
        gchar *txt = gtk_text_buffer_get_text(tb, &start, &end, FALSE);
        interview_record_answer(iv, cur, txt);
        g_free(txt);
        interview_evaluate_current_answer(dlg, iv);
    }
    int avg = interview_compute_average(iv);
    interview_finish(iv);

    char pdf_path[1024] = "";
    bool has_pdf = report_generate_session_pdf(iv, avg, pdf_path, sizeof(pdf_path));

    AuraDashboard *dash = (AuraDashboard *)g_object_get_data(G_OBJECT(dlg), "dash");
    interview_destroy(iv);
    g_object_set_data(G_OBJECT(dlg), "interview", NULL);
    gtk_widget_destroy(dlg);

    if (dash) {
        refresh_dashboard_metrics(dash);
        refresh_reports_page(dash);
        refresh_leaderboard_page(dash);
        refresh_profile_page(dash);
        char msg[768];
        if (has_pdf) {
            snprintf(msg, sizeof(msg),
                "Entretien termine !\nScore moyen: %d/10\n"
                "Resultats enregistres dans SQLite.\n"
                "Rapport PDF genere:\n%s", avg, pdf_path);
        } else {
            snprintf(msg, sizeof(msg),
                "Entretien termine !\nScore moyen: %d/10\nResultats enregistres dans SQLite.", avg);
        }
        GtkWidget *result = gtk_message_dialog_new(GTK_WINDOW(dash->window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, has_pdf ? GTK_BUTTONS_YES_NO : GTK_BUTTONS_OK,
            "%s", msg);
        if (has_pdf) {
            gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(result),
                "Ouvrir le rapport PDF maintenant ?");
        }
        gint resp = gtk_dialog_run(GTK_DIALOG(result));
        gtk_widget_destroy(result);
        if (has_pdf && resp == GTK_RESPONSE_YES)
            report_open_file(pdf_path);
    }
}

static void interview_update_progress(GtkWidget *dlg, Interview *iv) {
    GtkWidget *progress = GTK_WIDGET(g_object_get_data(G_OBJECT(dlg), "progress_label"));
    if (!progress || !iv) return;
    int idx = interview_get_current_index(iv);
    int total = interview_get_questions_count(iv);
    char buf[64];
    snprintf(buf, sizeof(buf), "Question %d / %d", idx + 1, total);
    gtk_label_set_text(GTK_LABEL(progress), buf);
}

static void interview_update_feedback(GtkWidget *dlg, Interview *iv, int idx) {
    GtkWidget *verdict_lbl = GTK_WIDGET(g_object_get_data(G_OBJECT(dlg), "verdict_label"));
    GtkWidget *detail_lbl = GTK_WIDGET(g_object_get_data(G_OBJECT(dlg), "feedback_detail"));
    if (!verdict_lbl || !detail_lbl || !iv || idx < 0) return;

    int v = interview_get_verdict(iv, idx);
    const char *fb = interview_get_feedback(iv, idx);
    if (v == VERDICT_CORRECT) {
        gtk_label_set_text(GTK_LABEL(verdict_lbl), "VRAI");
        gtk_style_context_remove_class(gtk_widget_get_style_context(verdict_lbl), "verdict-false");
        gtk_style_context_remove_class(gtk_widget_get_style_context(verdict_lbl), "verdict-partial");
        gtk_style_context_add_class(gtk_widget_get_style_context(verdict_lbl), "verdict-true");
    } else if (v == VERDICT_PARTIAL) {
        gtk_label_set_text(GTK_LABEL(verdict_lbl), "PARTIEL");
        gtk_style_context_remove_class(gtk_widget_get_style_context(verdict_lbl), "verdict-true");
        gtk_style_context_remove_class(gtk_widget_get_style_context(verdict_lbl), "verdict-false");
        gtk_style_context_add_class(gtk_widget_get_style_context(verdict_lbl), "verdict-partial");
    } else if (v == VERDICT_INCORRECT) {
        gtk_label_set_text(GTK_LABEL(verdict_lbl), "FAUX");
        gtk_style_context_remove_class(gtk_widget_get_style_context(verdict_lbl), "verdict-true");
        gtk_style_context_remove_class(gtk_widget_get_style_context(verdict_lbl), "verdict-partial");
        gtk_style_context_add_class(gtk_widget_get_style_context(verdict_lbl), "verdict-false");
    } else {
        gtk_label_set_text(GTK_LABEL(verdict_lbl), "—");
        gtk_style_context_remove_class(gtk_widget_get_style_context(verdict_lbl), "verdict-true");
        gtk_style_context_remove_class(gtk_widget_get_style_context(verdict_lbl), "verdict-false");
        gtk_style_context_remove_class(gtk_widget_get_style_context(verdict_lbl), "verdict-partial");
    }
    gtk_label_set_text(GTK_LABEL(detail_lbl), fb && fb[0] ? fb : "Repondez puis cliquez Valider ou Suivant pour voir la correction.");
    gtk_label_set_line_wrap(GTK_LABEL(detail_lbl), TRUE);
}

static void interview_evaluate_current_answer(GtkWidget *dlg, Interview *iv) {
    if (!iv) return;
    int cur = interview_get_current_index(iv);
    if (cur < 0) return;
    GtkWidget *answer_view = GTK_WIDGET(g_object_get_data(G_OBJECT(dlg), "answer_view"));
    if (answer_view) {
        GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(answer_view));
        GtkTextIter start, end;
        gtk_text_buffer_get_start_iter(tb, &start);
        gtk_text_buffer_get_end_iter(tb, &end);
        gchar *txt = gtk_text_buffer_get_text(tb, &start, &end, FALSE);
        interview_record_answer(iv, cur, txt);
        g_free(txt);
    }
    GtkWidget *eval_lbl = GTK_WIDGET(g_object_get_data(G_OBJECT(dlg), "eval_status"));
    if (eval_lbl) gtk_label_set_text(GTK_LABEL(eval_lbl), "Evaluation en cours...");
    while (gtk_events_pending()) gtk_main_iteration();
    interview_evaluate_current(iv, cur);
    if (eval_lbl) gtk_label_set_text(GTK_LABEL(eval_lbl), "");
    interview_update_feedback(dlg, iv, cur);
}

static void interview_dialog_show(AuraDashboard *dash) {
    if (!dash) return;
    questions_ensure_loaded();

    GtkWidget *dlg = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    add_class(dlg, "interview-window");
    gtk_window_set_transient_for(GTK_WINDOW(dlg), GTK_WINDOW(dash->window));
    gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
    gtk_window_set_title(GTK_WINDOW(dlg), "ESISA — Simulation d'entretien");
    gtk_window_set_default_size(GTK_WINDOW(dlg), 820, 580);

    GtkWidget *v = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(v), 16);
    gtk_container_add(GTK_CONTAINER(dlg), v);

    char header[256];
    snprintf(header, sizeof(header), "%s — Niveau %s",
        dash->selected_domain ? dash->selected_domain : "Algorithmique",
        dash->selected_level ? dash->selected_level : "Junior");
    GtkWidget *header_label = gtk_label_new(header);
    gtk_label_set_xalign(GTK_LABEL(header_label), 0.0f);
    add_class(header_label, "panel-heading");
    gtk_box_pack_start(GTK_BOX(v), header_label, FALSE, FALSE, 0);

    GtkWidget *progress_label = gtk_label_new("Question 1 / 5");
    gtk_label_set_xalign(GTK_LABEL(progress_label), 0.0f);
    add_class(progress_label, "dash-accent");
    gtk_box_pack_start(GTK_BOX(v), progress_label, FALSE, FALSE, 4);
    g_object_set_data(G_OBJECT(dlg), "progress_label", progress_label);

    GtkWidget *q_frame = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    add_class(q_frame, "glass-card");
    gtk_box_pack_start(GTK_BOX(v), q_frame, FALSE, FALSE, 0);

    GtkWidget *q_title = gtk_label_new("Question :");
    gtk_label_set_xalign(GTK_LABEL(q_title), 0.0f);
    add_class(q_title, "dash-accent");
    gtk_box_pack_start(GTK_BOX(q_frame), q_title, FALSE, FALSE, 0);

    GtkWidget *q_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(q_scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(q_scroll, -1, 100);
    GtkWidget *q_label = gtk_label_new("Chargement de la question...");
    gtk_label_set_line_wrap(GTK_LABEL(q_label), TRUE);
    gtk_label_set_xalign(GTK_LABEL(q_label), 0.0f);
    gtk_widget_set_margin_start(q_label, 4);
    gtk_widget_set_margin_end(q_label, 4);
    add_class(q_label, "question-card");
    gtk_container_add(GTK_CONTAINER(q_scroll), q_label);
    gtk_box_pack_start(GTK_BOX(q_frame), q_scroll, TRUE, TRUE, 0);

    GtkWidget *answer_title = gtk_label_new("Votre reponse :");
    gtk_label_set_xalign(GTK_LABEL(answer_title), 0.0f);
    add_class(answer_title, "dash-accent");
    gtk_widget_set_margin_top(answer_title, 8);
    gtk_box_pack_start(GTK_BOX(v), answer_title, FALSE, FALSE, 0);

    GtkWidget *answer_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(answer_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(answer_scroll, -1, 160);
    add_class(answer_scroll, "answer-scroll");
    GtkWidget *answer_view = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(answer_view), GTK_WRAP_WORD_CHAR);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(answer_view), 8);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(answer_view), 8);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(answer_view), 8);
    gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(answer_view), 8);
    add_class(answer_view, "answer-box");
    gtk_container_add(GTK_CONTAINER(answer_scroll), answer_view);
    gtk_box_pack_start(GTK_BOX(v), answer_scroll, TRUE, TRUE, 0);

    GtkWidget *verdict_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_top(verdict_row, 8);
    GtkWidget *verdict_title = gtk_label_new("Resultat :");
    add_class(verdict_title, "dash-accent");
    gtk_box_pack_start(GTK_BOX(verdict_row), verdict_title, FALSE, FALSE, 0);
    GtkWidget *verdict_label = gtk_label_new("—");
    add_class(verdict_label, "verdict-badge");
    gtk_box_pack_start(GTK_BOX(verdict_row), verdict_label, FALSE, FALSE, 0);
    GtkWidget *eval_status = gtk_label_new("");
    add_class(eval_status, "panel-copy");
    gtk_box_pack_start(GTK_BOX(verdict_row), eval_status, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(v), verdict_row, FALSE, FALSE, 0);
    g_object_set_data(G_OBJECT(dlg), "verdict_label", verdict_label);
    g_object_set_data(G_OBJECT(dlg), "eval_status", eval_status);

    GtkWidget *feedback_detail = gtk_label_new("Repondez puis cliquez Valider pour corriger sans changer de question.");
    gtk_label_set_xalign(GTK_LABEL(feedback_detail), 0.0f);
    gtk_label_set_line_wrap(GTK_LABEL(feedback_detail), TRUE);
    add_class(feedback_detail, "feedback-detail");
    gtk_box_pack_start(GTK_BOX(v), feedback_detail, FALSE, FALSE, 4);
    g_object_set_data(G_OBJECT(dlg), "feedback_detail", feedback_detail);

    GtkWidget *h = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_top(h, 12);
    gtk_box_pack_start(GTK_BOX(v), h, FALSE, FALSE, 0);

    GtkWidget *btn_prev = gtk_button_new_with_label("Precedent");
    GtkWidget *btn_validate = gtk_button_new_with_label("Valider");
    GtkWidget *btn_next = gtk_button_new_with_label("Suivant");
    GtkWidget *btn_finish = gtk_button_new_with_label("Terminer");
    GtkWidget *btn_close = gtk_button_new_with_label("Fermer");
    add_class(btn_prev, "secondary-action");
    add_class(btn_validate, "primary-action");
    add_class(btn_next, "primary-action");
    add_class(btn_finish, "primary-action");
    add_class(btn_close, "secondary-action");
    style_flat_button(btn_prev);
    style_flat_button(btn_validate);
    style_flat_button(btn_next);
    style_flat_button(btn_finish);
    style_flat_button(btn_close);
    gtk_box_pack_start(GTK_BOX(h), btn_prev, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(h), btn_validate, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(h), btn_next, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(h), btn_finish, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(h), btn_close, FALSE, FALSE, 0);

    Interview *iv = interview_create(
        dash->username ? dash->username : aura_session_username(),
        dash->selected_domain ? dash->selected_domain : "Algorithmique",
        dash->selected_level ? dash->selected_level : "Junior",
        5);
    g_object_set_data(G_OBJECT(dlg), "interview", iv);
    g_object_set_data(G_OBJECT(dlg), "dash", dash);
    g_object_set_data(G_OBJECT(dlg), "q_label", q_label);
    g_object_set_data(G_OBJECT(dlg), "answer_view", answer_view);

    g_signal_connect(btn_prev, "clicked", G_CALLBACK(interview_dialog_prev_cb), NULL);
    g_signal_connect(btn_validate, "clicked", G_CALLBACK(interview_dialog_validate_cb), NULL);
    g_signal_connect(btn_next, "clicked", G_CALLBACK(interview_dialog_next_cb), NULL);
    g_signal_connect(btn_finish, "clicked", G_CALLBACK(interview_dialog_finish_cb), NULL);
    g_signal_connect_swapped(btn_close, "clicked", G_CALLBACK(gtk_widget_destroy), dlg);

    char qbuf[4096];
    if (interview_next(iv, qbuf, sizeof(qbuf))) {
        gtk_label_set_text(GTK_LABEL(q_label), qbuf);
    } else {
        gtk_label_set_text(GTK_LABEL(q_label), "Aucune question disponible pour cette matiere.");
    }
    interview_dialog_update(dlg, q_label, answer_view);
    interview_update_progress(dlg, iv);

    gtk_widget_show_all(dlg);
    gtk_window_present(GTK_WINDOW(dlg));
    gtk_widget_grab_focus(answer_view);
}

static void picker_highlight_siblings(GtkWidget *selected_btn) {
    GtkWidget *parent = gtk_widget_get_parent(selected_btn);
    if (!parent) return;
    GList *kids = gtk_container_get_children(GTK_CONTAINER(parent));
    for (GList *l = kids; l; l = l->next) {
        GtkWidget *c = GTK_WIDGET(l->data);
        if (c == selected_btn) add_class(c, "active");
        else gtk_style_context_remove_class(gtk_widget_get_style_context(c), "active");
    }
    g_list_free(kids);
}

static void on_level_button_clicked(GtkWidget *widget, gpointer user_data) {
    AuraDashboard *dash = (AuraDashboard *)user_data;
    const gchar *level = (const gchar *)g_object_get_data(G_OBJECT(widget), "level");
    if (!dash) return;
    if (dash->selected_level) g_free(dash->selected_level);
    dash->selected_level = g_strdup(level ? level : "Junior");
    picker_highlight_siblings(widget);
}

static void on_level_continue_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    GtkWidget *dlg = GTK_WIDGET(user_data);
    AuraDashboard *dash = g_object_get_data(G_OBJECT(dlg), "dash");
    if (!dash) return;
    if (!dash->selected_level || !dash->selected_level[0]) {
        if (dash->selected_level) g_free(dash->selected_level);
        dash->selected_level = g_strdup("Junior");
    }
    gtk_widget_destroy(dlg);
    interview_dialog_show(dash);
}

static void on_domain_button_clicked(GtkWidget *widget, gpointer user_data) {
    AuraDashboard *dash = (AuraDashboard *)user_data;
    const gchar *domain = (const gchar *)g_object_get_data(G_OBJECT(widget), "domain");
    if (!dash) return;
    if (dash->selected_domain) g_free(dash->selected_domain);
    dash->selected_domain = g_strdup(domain ? domain : "Algorithmique");
    picker_highlight_siblings(widget);
}

static void on_domain_continue_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    GtkWidget *dlg = GTK_WIDGET(user_data);
    AuraDashboard *dash = g_object_get_data(G_OBJECT(dlg), "dash");
    if (!dash) return;
    if (!dash->selected_domain || !dash->selected_domain[0]) {
        dash->selected_domain = g_strdup("Algorithmique");
    }
    gtk_widget_destroy(dlg);
    level_dialog_show(dash);
}

static void domain_dialog_show(AuraDashboard *dash) {
    if (!dash) return;
    GtkWidget *dlg = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    add_class(dlg, "picker-window");
    gtk_window_set_transient_for(GTK_WINDOW(dlg), GTK_WINDOW(dash->window));
    gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
    gtk_window_set_title(GTK_WINDOW(dlg), "Choisir la matiere");
    gtk_window_set_default_size(GTK_WINDOW(dlg), 820, 300);

    GtkWidget *v = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(v), 12);
    gtk_container_add(GTK_CONTAINER(dlg), v);

    GtkWidget *title = gtk_label_new("Selectionnez une matiere d'entretien");
    add_class(title, "panel-heading");
    gtk_box_pack_start(GTK_BOX(v), title, FALSE, FALSE, 0);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_box_pack_start(GTK_BOX(v), grid, TRUE, TRUE, 0);

    const gchar *domains[] = {
        "Architecture des ordinateurs",
        "Algorithmique",
        "Programmation C"
    };
    for (int i = 0; i < 3; ++i) {
        GtkWidget *btn = gtk_button_new_with_label(domains[i]);
        gtk_button_set_relief(GTK_BUTTON(btn), GTK_RELIEF_NONE);
        add_class(btn, "hero-badge");
        gtk_widget_set_size_request(btn, 240, 100);
        g_object_set_data(G_OBJECT(btn), "domain", (gpointer)domains[i]);
        g_signal_connect(btn, "clicked", G_CALLBACK(on_domain_button_clicked), dash);
        gtk_grid_attach(GTK_GRID(grid), btn, i, 0, 1, 1);
        if (i == 1) {
            if (dash->selected_domain) g_free(dash->selected_domain);
            dash->selected_domain = g_strdup(domains[i]);
            add_class(btn, "active");
        }
    }

    GtkWidget *btn_continue = gtk_button_new_with_label("Continuer");
    gtk_button_set_relief(GTK_BUTTON(btn_continue), GTK_RELIEF_NONE);
    add_class(btn_continue, "primary-action");
    g_object_set_data(G_OBJECT(dlg), "dash", dash);
    g_signal_connect(btn_continue, "clicked", G_CALLBACK(on_domain_continue_clicked), dlg);
    gtk_box_pack_end(GTK_BOX(v), btn_continue, FALSE, FALSE, 0);

    gtk_widget_show_all(dlg);
}

static void level_dialog_show(AuraDashboard *dash) {
    if (!dash) return;
    GtkWidget *dlg = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    add_class(dlg, "picker-window");
    gtk_window_set_transient_for(GTK_WINDOW(dlg), GTK_WINDOW(dash->window));
    gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
    gtk_window_set_title(GTK_WINDOW(dlg), "Choisir le niveau");
    gtk_window_set_default_size(GTK_WINDOW(dlg), 760, 360);

    GtkWidget *v = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(v), 12);
    gtk_container_add(GTK_CONTAINER(dlg), v);

    GtkWidget *level_title = gtk_label_new("Cliquez un niveau puis Continuer pour demarrer l'entretien");
    gtk_label_set_xalign(GTK_LABEL(level_title), 0.0f);
    add_class(level_title, "panel-copy");
    gtk_box_pack_start(GTK_BOX(v), level_title, FALSE, FALSE, 0);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_box_pack_start(GTK_BOX(v), grid, TRUE, TRUE, 0);

    const gchar *levels[] = {"Junior", "Intermediate", "Advanced", "Expert"};
    for (int i = 0; i < 4; ++i) {
        GtkWidget *btn = gtk_button_new_with_label(levels[i]);
        gtk_button_set_relief(GTK_BUTTON(btn), GTK_RELIEF_NONE);
        add_class(btn, "hero-badge");
        gtk_widget_set_size_request(btn, 160, 100);
        g_object_set_data(G_OBJECT(btn), "level", (gpointer)levels[i]);
        g_signal_connect(btn, "clicked", G_CALLBACK(on_level_button_clicked), dash);
        gtk_grid_attach(GTK_GRID(grid), btn, i, 0, 1, 1);
        if (i == 0) {
            if (dash->selected_level) g_free(dash->selected_level);
            dash->selected_level = g_strdup(levels[i]);
            add_class(btn, "active");
        }
    }

    GtkWidget *h = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_pack_start(GTK_BOX(v), h, FALSE, FALSE, 0);
    GtkWidget *btn_continue = gtk_button_new_with_label("Continuer");
    gtk_button_set_relief(GTK_BUTTON(btn_continue), GTK_RELIEF_NONE);
    add_class(btn_continue, "primary-action");
    gtk_box_pack_end(GTK_BOX(h), btn_continue, FALSE, FALSE, 0);

    g_object_set_data(G_OBJECT(dlg), "dash", dash);
    g_signal_connect(btn_continue, "clicked", G_CALLBACK(on_level_continue_clicked), dlg);

    gtk_widget_show_all(dlg);
}

static void launch_interview_handler(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AuraDashboard *dash = (AuraDashboard *)user_data;
    if (!dash) return;
    domain_dialog_show(dash);
}

static void apply_dashboard_css(void) {
    GtkCssProvider *provider = gtk_css_provider_new();
    const gchar *css =
        "* { font-family: 'Segoe UI', 'Inter', sans-serif; }\n"
        "window.dashboard-window, window.interview-window, window.picker-window { background-color: #12101A; color: #F5F0E8; }\n"
        "label { color: #D4C8E8; }\n"
        "entry { background-color: rgba(26, 22, 40, 0.95); color: #F5F0E8; }\n"
        "textview { background-color: rgba(26, 22, 40, 0.95); color: #F5F0E8; }\n"
        "textview text { background-color: rgba(26, 22, 40, 0.95); color: #F5F0E8; }\n"
        ".answer-scroll { background-color: rgba(26, 22, 40, 0.95); border: 2px solid rgba(124, 108, 240, 0.3); border-radius: 12px; }\n"
        ".verdict-badge { font-size: 18px; font-weight: 900; padding: 4px 14px; border-radius: 8px; }\n"
        ".verdict-true { color: #4ADE80; background: rgba(74, 222, 128, 0.15); }\n"
        ".verdict-false { color: #F87171; background: rgba(248, 113, 113, 0.15); }\n"
        ".verdict-partial { color: #FBBF24; background: rgba(251, 191, 36, 0.15); }\n"
        ".feedback-detail { color: #A89BB8; font-size: 13px; }\n"
        ".leaderboard-gold { color: #E8A838; font-weight: 700; }\n"
        ".question-card { color: #F5F0E8; font-size: 15px; font-weight: 600; line-height: 1.5; }\n"
        ".answer-box { color: #F5F0E8; font-size: 14px; }\n"
        ".hero-badge:hover, button.hero-badge:hover {"
        "  background-color: rgba(124, 108, 240, 0.25);"
        "  border-color: rgba(124, 108, 240, 0.6);"
        "}"
        ".hero-badge.active, button.hero-badge.active {"
        "  background-color: rgba(232, 168, 56, 0.25);"
        "  border-color: rgba(232, 168, 56, 0.7);"
        "}"
        ".hero-badge.active label, button.hero-badge.active label { color: #FFF8EC; }\n"
        ".dashboard-root { background: linear-gradient(135deg, #12101A 0%, #1A1628 50%, #12101A 100%); }\n"
        ".sidebar {"
        "  background: linear-gradient(180deg, #1A1628 0%, #14111F 100%);"
        "  border-right: 1px solid rgba(232, 168, 56, 0.15);"
        "  padding: 20px 14px;"
        "  box-shadow: 4px 0 24px rgba(0, 0, 0, 0.25);"
        "}\n"
        "button { background-image: none; box-shadow: none; }\n"
        "scrolledwindow, scrolledwindow viewport { background-color: rgba(30, 26, 46, 0.92); }\n"
        "button.hero-badge, .hero-badge {"
        "  background-color: rgba(42, 37, 64, 0.95);"
        "  border: 2px solid rgba(124, 108, 240, 0.35);"
        "  border-radius: 14px;"
        "  padding: 16px;"
        "  color: #F5F0E8;"
        "}"
        "button.hero-badge label, .hero-badge label { color: #F5F0E8; font-weight: 700; }\n"
        "button.nav-button, .nav-button {"
        "  background-image: none;"
        "  background-color: rgba(42, 37, 64, 0.85);"
        "  color: #D4C8E8;"
        "  border: 1px solid rgba(124, 108, 240, 0.18);"
        "  border-radius: 12px;"
        "  padding: 13px 16px;"
        "  font-size: 13px;"
        "  font-weight: 600;"
        "  letter-spacing: 0.3px;"
        "  box-shadow: none;"
        "}\n"
        "button.nav-button label, .nav-button label { color: #D4C8E8; }\n"
        ".nav-button:hover {"
        "  background-color: rgba(124, 108, 240, 0.22);"
        "  border-color: rgba(124, 108, 240, 0.45);"
        "  color: #FFFFFF;"
        "}\n"
        ".nav-button.active {"
        "  background: linear-gradient(135deg, rgba(232, 168, 56, 0.28) 0%, rgba(124, 108, 240, 0.22) 100%);"
        "  border-color: rgba(232, 168, 56, 0.55);"
        "  color: #FFF8EC;"
        "  box-shadow: 0 4px 16px rgba(232, 168, 56, 0.18);"
        "}\n"
        ".nav-button.active label { color: #FFF8EC; font-weight: 700; }\n"
        "button.primary-action, .primary-action {"
        "  background-image: linear-gradient(135deg, #E8A838 0%, #D4922A 100%);"
        "  background-color: #E8A838;"
        "  color: #1A1208;"
        "  border: none;"
        "  border-radius: 14px;"
        "  padding: 16px 28px;"
        "  font-size: 15px;"
        "  font-weight: 800;"
        "  letter-spacing: 0.5px;"
        "  box-shadow: 0 8px 24px rgba(232, 168, 56, 0.35);"
        "}\n"
        "button.primary-action label, .primary-action label { color: #1A1208; font-weight: 800; }\n"
        ".primary-action:hover {"
        "  background-image: linear-gradient(135deg, #F0B84A 0%, #E8A838 100%);"
        "  box-shadow: 0 12px 32px rgba(232, 168, 56, 0.45);"
        "}\n"
        "button.secondary-action, .secondary-action {"
        "  background-image: none;"
        "  background-color: rgba(42, 37, 64, 0.9);"
        "  color: #C4B8D4;"
        "  border: 1px solid rgba(124, 108, 240, 0.25);"
        "  border-radius: 12px;"
        "  padding: 12px 16px;"
        "  font-size: 13px;"
        "  font-weight: 600;"
        "}\n"
        "button.secondary-action label, .secondary-action label { color: #C4B8D4; }\n"
        ".secondary-action:hover {"
        "  background-color: rgba(124, 108, 240, 0.25);"
        "  border-color: rgba(124, 108, 240, 0.5);"
        "  color: #FFFFFF;"
        "}\n"
        ".neon-title {"
        "  color: #E8A838;"
        "  font-weight: 900;"
        "  font-size: 22px;"
        "  letter-spacing: 4px;"
        "  text-shadow: 0 0 20px rgba(232, 168, 56, 0.3);"
        "}\n"
        ".glass-card {"
        "  background-color: rgba(30, 26, 46, 0.92);"
        "  border: 1px solid rgba(124, 108, 240, 0.2);"
        "  border-radius: 16px;"
        "  padding: 18px;"
        "  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.2);"
        "}\n"
        ".panel-heading { color: #F5F0E8; font-size: 26px; font-weight: 900; letter-spacing: -0.5px; }\n"
        ".panel-copy { color: #A89BB8; font-size: 14px; line-height: 1.5; }\n"
        ".stat-value { color: #E8A838; font-size: 28px; font-weight: 900; }\n"
        ".stat-label { color: #8B7FA0; font-size: 11px; font-weight: 600; letter-spacing: 1px; text-transform: uppercase; }\n"
        ".dash-muted { color: #A89BB8; font-size: 13px; }\n"
        ".dash-accent { color: #7C6CF0; font-size: 13px; font-weight: 600; }\n"
        ".dash-footer { color: #6B5F7A; font-size: 11px; }\n"
        "progressbar trough { background-color: rgba(42, 37, 64, 0.8); border-radius: 8px; min-height: 10px; }\n"
        "progressbar progress {"
        "  background-image: linear-gradient(90deg, #E8A838 0%, #7C6CF0 100%);"
        "  border-radius: 8px;"
        "}\n";

    gtk_css_provider_load_from_data(provider, css, -1, NULL);
    GdkScreen *screen = gdk_screen_get_default();
    if (screen) {
        gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
    g_object_unref(provider);
}

/* Helper to add CSS class (copy from login_ui style helper) */
static void add_class(GtkWidget *widget, const char *class_name) {
    gtk_style_context_add_class(gtk_widget_get_style_context(widget), class_name);
}

static GtkWidget *make_nav_button(AuraDashboard *dash, GtkWidget *parent, const gchar *label_text, const gchar *route, gboolean active) {
    GtkWidget *btn = gtk_button_new_with_label(label_text);
    gtk_button_set_relief(GTK_BUTTON(btn), GTK_RELIEF_NONE);
    add_class(btn, "nav-button");
    if (active) add_class(btn, "active");
    gtk_widget_set_halign(btn, GTK_ALIGN_FILL);
    gtk_widget_set_hexpand(btn, TRUE);
    gtk_box_pack_start(GTK_BOX(parent), btn, FALSE, FALSE, 6);
    connect_dashboard_button(btn);
    g_object_set_data(G_OBJECT(btn), "route", (gpointer)route);
    g_signal_connect(btn, "clicked", G_CALLBACK(on_dashboard_nav_clicked), dash);
    return btn;
}

static gboolean dashboard_tick_cb(gpointer user_data) {
    AuraDashboard *dash = (AuraDashboard *)user_data;
    dash->elapsed_seconds++;

    if (dash->elapsed_seconds % 10 == 0) {
        refresh_dashboard_metrics(dash);
    }

    Statistics stats = stats_get_user_stats(dash->username);
    gdouble p = stats.average_score > 0 ? (stats.average_score / 10.0) : 0.0;
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(dash->progress_bar), p);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(dash->progress_bar), "Progression moyenne");

    gchar status[128];
    snprintf(status, sizeof(status), "IA Groq: active — session %ds", dash->elapsed_seconds);
    if (GTK_IS_LABEL(dash->ai_status_label)) gtk_label_set_text(GTK_LABEL(dash->ai_status_label), status);

    gchar sc[64];
    snprintf(sc, sizeof(sc), "Entretiens: %d", stats.interviews_taken);
    if (GTK_IS_LABEL(dash->session_counter_label)) gtk_label_set_text(GTK_LABEL(dash->session_counter_label), sc);

    return G_SOURCE_CONTINUE;
}

gboolean aura_launch_dashboard(int *argc, char ***argv) {
    (void)argc;
    (void)argv;

    apply_dashboard_css();

    AuraDashboard *dash = g_new0(AuraDashboard, 1);
    dash->elapsed_seconds = 0;
    dash->selected_level = NULL;
    g_dashboard_restart_requested = FALSE;

    dash->username = g_strdup(aura_session_username());
    dash->interactions_enabled = TRUE;

    dash->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(dash->window), "ESISA — Simulateur d'entretiens");
    gtk_window_set_default_size(GTK_WINDOW(dash->window), 1280, 820);
    gtk_window_set_position(GTK_WINDOW(dash->window), GTK_WIN_POS_CENTER);
    add_class(dash->window, "dashboard-window");

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    add_class(root, "dashboard-root");
    gtk_container_add(GTK_CONTAINER(dash->window), root);

    // Sidebar
    dash->sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    add_class(dash->sidebar, "sidebar");
    gtk_widget_set_size_request(dash->sidebar, 220, -1);
    gtk_box_pack_start(GTK_BOX(root), dash->sidebar, FALSE, FALSE, 0);

    GtkWidget *brand = gtk_label_new("\nESISA\nInterview\n");
    add_class(brand, "neon-title");
    gtk_widget_set_halign(brand, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(dash->sidebar), brand, FALSE, FALSE, 8);

    dash->nav_buttons[0] = make_nav_button(dash, dash->sidebar, "Accueil", "overview", TRUE);
    dash->nav_buttons[1] = make_nav_button(dash, dash->sidebar, "Rapports", "reports", FALSE);
    dash->nav_buttons[2] = make_nav_button(dash, dash->sidebar, "Classement", "leaderboard", FALSE);
    dash->nav_buttons[3] = make_nav_button(dash, dash->sidebar, "Profil", "profile", FALSE);
    dash->nav_buttons[4] = make_nav_button(dash, dash->sidebar, "Parametres", "settings", FALSE);

    GtkWidget *sidebar_spacer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(dash->sidebar), sidebar_spacer, TRUE, TRUE, 0);

    GtkWidget *logout_btn = gtk_button_new_with_label("Deconnexion");
    gtk_button_set_relief(GTK_BUTTON(logout_btn), GTK_RELIEF_NONE);
    add_class(logout_btn, "secondary-action");
    gtk_widget_set_hexpand(logout_btn, TRUE);
    gtk_box_pack_start(GTK_BOX(dash->sidebar), logout_btn, FALSE, FALSE, 6);
    connect_dashboard_button(logout_btn);
    g_object_set_data(G_OBJECT(logout_btn), "route", (gpointer)"logout");
    g_signal_connect(logout_btn, "clicked", G_CALLBACK(on_dashboard_nav_clicked), dash);

    GtkWidget *exit_btn = gtk_button_new_with_label("Quitter");
    gtk_button_set_relief(GTK_BUTTON(exit_btn), GTK_RELIEF_NONE);
    add_class(exit_btn, "secondary-action");
    gtk_widget_set_hexpand(exit_btn, TRUE);
    gtk_box_pack_start(GTK_BOX(dash->sidebar), exit_btn, FALSE, FALSE, 6);
    connect_dashboard_button(exit_btn);
    g_object_set_data(G_OBJECT(exit_btn), "route", (gpointer)"exit");
    g_signal_connect(exit_btn, "clicked", G_CALLBACK(on_dashboard_nav_clicked), dash);

    // Main content area as a routed stack
    dash->stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(dash->stack), GTK_STACK_TRANSITION_TYPE_CROSSFADE);
    gtk_stack_set_transition_duration(GTK_STACK(dash->stack), 240);
    gtk_box_pack_start(GTK_BOX(root), dash->stack, TRUE, TRUE, 18);

    GtkWidget *overview_page = create_dashboard_page(
        "Accueil",
        "Lancez un entretien simule, suivez votre progression et consultez vos scores en temps reel.",
        NULL);
    GtkWidget *reports_page = create_dashboard_page(
        "Rapports de performance",
        "Historique de vos entretiens et scores par matiere.",
        &dash->reports_container);
    GtkWidget *leaderboard_page = create_dashboard_page(
        "Classement",
        "Meilleurs scores par matiere parmi tous les etudiants.",
        &dash->leaderboard_container);
    GtkWidget *profile_page = create_dashboard_page(
        "Profil etudiant",
        "Informations du compte et statistiques personnelles.",
        &dash->profile_container);
    GtkWidget *settings_page = create_dashboard_page(
        "Parametres",
        "Configuration API IA, base de donnees et matieres disponibles.",
        &dash->settings_container);

    gtk_stack_add_named(GTK_STACK(dash->stack), overview_page, "overview");
    gtk_stack_add_named(GTK_STACK(dash->stack), reports_page, "reports");
    gtk_stack_add_named(GTK_STACK(dash->stack), leaderboard_page, "leaderboard");
    gtk_stack_add_named(GTK_STACK(dash->stack), profile_page, "profile");
    gtk_stack_add_named(GTK_STACK(dash->stack), settings_page, "settings");

    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_box_pack_start(GTK_BOX(overview_page), content, TRUE, TRUE, 0);

    // Quick launch interview button
    GtkWidget *launch_interview_btn = gtk_button_new_with_label("Lancer un entretien");
    gtk_button_set_relief(GTK_BUTTON(launch_interview_btn), GTK_RELIEF_NONE);
    add_class(launch_interview_btn, "primary-action");
    gtk_box_pack_start(GTK_BOX(content), launch_interview_btn, FALSE, FALSE, 0);
    g_signal_connect(launch_interview_btn, "clicked", G_CALLBACK(launch_interview_handler), dash);

    // Top row: welcome + AI status
    GtkWidget *top_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_box_pack_start(GTK_BOX(content), top_row, FALSE, FALSE, 0);

    dash->welcome_label = gtk_label_new("Bienvenue — simulateur d'entretiens ESISA");
    add_class(dash->welcome_label, "dash-muted");
    gtk_widget_set_halign(dash->welcome_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(top_row), dash->welcome_label, TRUE, TRUE, 0);

    dash->ai_status_label = gtk_label_new("IA Groq: prete");
    add_class(dash->ai_status_label, "dash-accent");
    gtk_widget_set_halign(dash->ai_status_label, GTK_ALIGN_END);
    gtk_box_pack_start(GTK_BOX(top_row), dash->ai_status_label, FALSE, FALSE, 4);

    // Stats cards
    GtkWidget *cards = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(cards), 12);
    gtk_grid_set_column_spacing(GTK_GRID(cards), 12);
    gtk_box_pack_start(GTK_BOX(content), cards, FALSE, FALSE, 0);

    const gchar *stat_labels[] = {"Entretiens", "Moyenne /10", "Total global", "Session (min)"};
    for (int i = 0; i < 4; i++) {
        GtkWidget *card = gtk_event_box_new();
        add_class(card, "glass-card");
        GtkWidget *v = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        GtkWidget *value = gtk_label_new("0");
        add_class(value, "stat-value");
        GtkWidget *label = gtk_label_new(stat_labels[i]);
        add_class(label, "stat-label");
        gtk_box_pack_start(GTK_BOX(v), value, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(v), label, FALSE, FALSE, 0);
        gtk_container_add(GTK_CONTAINER(card), v);
        gtk_grid_attach(GTK_GRID(cards), card, i, 0, 1, 1);
        dash->stat_cards[i] = card;
    }

    // Middle: progress and overview
    GtkWidget *mid = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_box_pack_start(GTK_BOX(content), mid, FALSE, FALSE, 0);

    GtkWidget *left_col = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_box_pack_start(GTK_BOX(mid), left_col, TRUE, TRUE, 0);

    GtkWidget *progress_card = gtk_event_box_new(); add_class(progress_card, "glass-card");
    GtkWidget *pbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    dash->progress_bar = gtk_progress_bar_new();
    gtk_box_pack_start(GTK_BOX(pbox), dash->progress_bar, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(progress_card), pbox);
    gtk_box_pack_start(GTK_BOX(left_col), progress_card, FALSE, FALSE, 0);

    GtkWidget *counters_card = gtk_event_box_new(); add_class(counters_card, "glass-card");
    GtkWidget *cb = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    dash->session_counter_label = gtk_label_new("Entretiens: 0");
    gtk_box_pack_start(GTK_BOX(cb), dash->session_counter_label, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(counters_card), cb);
    gtk_box_pack_start(GTK_BOX(left_col), counters_card, FALSE, FALSE, 0);

    // User panel on the right
    GtkWidget *user_panel = gtk_event_box_new(); add_class(user_panel, "glass-card");
    GtkWidget *up = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    char uname_buf[192];
    snprintf(uname_buf, sizeof(uname_buf), "Etudiant: %s",
        g_aura_session.fullname[0] ? g_aura_session.fullname : dash->username);
    GtkWidget *uname = gtk_label_new(uname_buf);
    gtk_box_pack_start(GTK_BOX(up), uname, FALSE, FALSE, 0);
    char email_buf[192];
    snprintf(email_buf, sizeof(email_buf), "Email: %s", dash->username ? dash->username : "—");
    GtkWidget *uemail = gtk_label_new(email_buf);
    gtk_box_pack_start(GTK_BOX(up), uemail, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(user_panel), up);
    gtk_box_pack_start(GTK_BOX(mid), user_panel, FALSE, FALSE, 0);

    // Footer quick links (small)
    GtkWidget *footer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_box_pack_start(GTK_BOX(content), footer, FALSE, FALSE, 0);
    GtkWidget *t = gtk_label_new("ESISA — Simulateur d'entretiens (Archi / Algo / C)");
    add_class(t, "dash-footer");
    gtk_box_pack_start(GTK_BOX(footer), t, FALSE, FALSE, 0);

    gtk_stack_set_visible_child_name(GTK_STACK(dash->stack), "overview");

    refresh_dashboard_metrics(dash);
    refresh_reports_page(dash);
    refresh_leaderboard_page(dash);
    refresh_profile_page(dash);
    refresh_settings_page(dash);

    dash->tick_id = g_timeout_add_seconds(1, dashboard_tick_cb, dash);

    g_signal_connect(dash->window, "destroy", G_CALLBACK(on_dashboard_destroy), dash);

    gtk_widget_show_all(dash->window);
    gtk_window_present(GTK_WINDOW(dash->window));
    gtk_main();

    gboolean relaunch_login = dash->relaunch_login;
    g_free(dash->username);
    g_free(dash->selected_level);
    g_free(dash->selected_domain);
    g_free(dash->lb_domain_filter);
    g_free(dash->lb_level_filter);
    g_free(dash);
    return relaunch_login;
}
