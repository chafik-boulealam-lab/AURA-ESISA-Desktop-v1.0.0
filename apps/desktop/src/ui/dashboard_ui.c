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
#include "aura_theme.h"

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
static void on_domain_button_clicked(GtkWidget *widget, gpointer user_data);

static void style_flat_button(GtkWidget *btn) {
    if (btn && GTK_IS_BUTTON(btn)) {
        gtk_button_set_relief(GTK_BUTTON(btn), GTK_RELIEF_NONE);
    }
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

    char dbgpath[1024];
    FILE *dbg = aura_fs_get_data_path("login_screen_debug.log", dbgpath, sizeof(dbgpath))
        ? fopen(dbgpath, "a") : NULL;
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

    GtkWidget *card = gtk_event_box_new();
    add_class(card, "glass-card");
    gtk_widget_set_margin_top(card, 20);
    gtk_widget_set_margin_bottom(card, 20);
    gtk_widget_set_margin_start(card, 20);
    gtk_widget_set_margin_end(card, 20);
    gtk_box_pack_start(GTK_BOX(page), card, TRUE, TRUE, 0);

    GtkWidget *card_inner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    gtk_container_set_border_width(GTK_CONTAINER(card_inner), 24);
    gtk_container_add(GTK_CONTAINER(card), card_inner);

    GtkWidget *title_label = gtk_label_new(title);
    gtk_label_set_xalign(GTK_LABEL(title_label), 0.0f);
    add_class(title_label, "panel-heading");
    gtk_box_pack_start(GTK_BOX(card_inner), title_label, FALSE, FALSE, 0);

    GtkWidget *copy_label = gtk_label_new(copy);
    gtk_label_set_xalign(GTK_LABEL(copy_label), 0.0f);
    gtk_label_set_line_wrap(GTK_LABEL(copy_label), TRUE);
    add_class(copy_label, "panel-copy");
    gtk_box_pack_start(GTK_BOX(card_inner), copy_label, FALSE, FALSE, 0);

    if (body_out) {
        GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
            GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll), GTK_SHADOW_NONE);
        add_class(scroll, "dash-scroll");
        gtk_widget_set_vexpand(scroll, TRUE);
        gtk_widget_set_hexpand(scroll, TRUE);
        gtk_widget_set_size_request(scroll, -1, 280);

        GtkWidget *body = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_widget_set_valign(body, GTK_ALIGN_START);
        gtk_container_add(GTK_CONTAINER(scroll), body);
        gtk_widget_show(body);

        gtk_box_pack_start(GTK_BOX(card_inner), scroll, TRUE, TRUE, 8);
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
    GtkWidget *visible = gtk_stack_get_visible_child(GTK_STACK(dash->stack));
    if (visible) {
        gtk_widget_show_all(visible);
        gtk_widget_queue_resize(visible);
    }
    set_active_nav(dash, button);
}

static void clear_container_children(GtkWidget *container) {
    if (!container) return;
    GList *children = gtk_container_get_children(GTK_CONTAINER(container));
    for (GList *l = children; l; l = l->next) gtk_widget_destroy(GTK_WIDGET(l->data));
    g_list_free(children);
}

static GtkWidget *dash_add_line(GtkWidget *box, const char *text, const char *css_class) {
    GtkWidget *lbl = gtk_label_new(text ? text : "");
    gtk_label_set_xalign(GTK_LABEL(lbl), 0.0f);
    gtk_label_set_line_wrap(GTK_LABEL(lbl), TRUE);
    gtk_widget_set_margin_start(lbl, 8);
    gtk_widget_set_margin_end(lbl, 8);
    add_class(lbl, css_class ? css_class : "panel-copy");
    gtk_box_pack_start(GTK_BOX(box), lbl, FALSE, FALSE, 6);
    gtk_widget_show(lbl);
    return lbl;
}

static GtkWidget *dash_add_section(GtkWidget *box, const char *title) {
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    add_class(sep, "section-sep");
    gtk_widget_set_margin_top(sep, 14);
    gtk_widget_set_margin_bottom(sep, 6);
    gtk_box_pack_start(GTK_BOX(box), sep, FALSE, FALSE, 0);
    gtk_widget_show(sep);
    return dash_add_line(box, title, "section-title");
}

static void dash_page_show(GtkWidget *body) {
    if (!body) return;
    gtk_widget_show_all(body);
    for (GtkWidget *p = body; p != NULL; p = gtk_widget_get_parent(p)) {
        gtk_widget_show(p);
        if (GTK_IS_CONTAINER(p))
            gtk_container_check_resize(GTK_CONTAINER(p));
    }
    gtk_widget_queue_resize(body);
}

static void on_report_pdf_open(GtkWidget *widget, gpointer user_data) {
    (void)user_data;
    const char *path = (const char *)g_object_get_data(G_OBJECT(widget), "pdf_path");
    if (path) report_open_file(path);
}

static void refresh_reports_page(AuraDashboard *dash) {
    if (!dash || !dash->reports_container) return;
    clear_container_children(dash->reports_container);

    dash_add_section(dash->reports_container, "Historique SQLite");

    DbScoreRow rows[20];
    int n = db_fetch_recent_scores(rows, 20, dash->username);
    if (n == 0) {
        dash_add_line(dash->reports_container,
            "Aucun entretien enregistre. Lancez un entretien depuis l'accueil.", "panel-copy");
    } else {
        for (int i = 0; i < n; i++) {
            char line[256];
            snprintf(line, sizeof(line), "%s — %d/10 — %s",
                rows[i].user_name, rows[i].score, rows[i].categorie);
            const char *score_class = rows[i].score >= 7 ? "score-high"
                : rows[i].score >= 5 ? "score-mid" : "score-low";
            GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
            add_class(row, "report-row");
            char score_part[32];
            snprintf(score_part, sizeof(score_part), "%d/10", rows[i].score);
            char full[256];
            snprintf(full, sizeof(full), "%s — %s — %s",
                rows[i].user_name, score_part, rows[i].categorie);
            GtkWidget *lbl = gtk_label_new(full);
            gtk_label_set_xalign(GTK_LABEL(lbl), 0.0f);
            add_class(lbl, "report-filename");
            gtk_box_pack_start(GTK_BOX(row), lbl, TRUE, TRUE, 8);
            GtkWidget *badge = gtk_label_new(score_part);
            add_class(badge, score_class);
            gtk_box_pack_end(GTK_BOX(row), badge, FALSE, FALSE, 8);
            gtk_box_pack_start(GTK_BOX(dash->reports_container), row, FALSE, FALSE, 4);
            gtk_widget_show_all(row);
        }
    }

    dash_add_section(dash->reports_container, "Rapports PDF (par session)");

    char pdf_paths[10][1024];
    int pn = report_list_pdfs(pdf_paths, 10);
    if (pn == 0) {
        dash_add_line(dash->reports_container,
            "Aucun PDF pour le moment — terminez un entretien pour generer un rapport.", "panel-copy");
        dash_page_show(dash->reports_container);
        return;
    }
    for (int i = 0; i < pn; i++) {
        const char *slash = strrchr(pdf_paths[i], '/');
        if (!slash) slash = strrchr(pdf_paths[i], '\\');
        const char *name = slash ? slash + 1 : pdf_paths[i];
        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        add_class(row, "report-row");
        gtk_widget_set_margin_start(row, 4);
        gtk_widget_set_margin_end(row, 4);
        GtkWidget *lbl = gtk_label_new(name);
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.0f);
        add_class(lbl, "report-filename");
        gtk_box_pack_start(GTK_BOX(row), lbl, TRUE, TRUE, 0);
        const char *btn_label = g_str_has_suffix(name, ".pdf") ? "Ouvrir PDF" : "Ouvrir TXT";
        GtkWidget *btn = gtk_button_new_with_label(btn_label);
        style_flat_button(btn);
        add_class(btn, "secondary-action");
        g_object_set_data_full(G_OBJECT(btn), "pdf_path", g_strdup(pdf_paths[i]), g_free);
        g_signal_connect(btn, "clicked", G_CALLBACK(on_report_pdf_open), NULL);
        gtk_box_pack_end(GTK_BOX(row), btn, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(dash->reports_container), row, FALSE, FALSE, 4);
        gtk_widget_show_all(row);
    }
    dash_page_show(dash->reports_container);
}

static GtkWidget *make_filter_chip(AuraDashboard *dash, GtkWidget *parent,
    const char *label, const char *filter_kind, const char *filter_value) {
    GtkWidget *btn = gtk_button_new_with_label(label);
    style_flat_button(btn);
    add_class(btn, "secondary-action");
    add_class(btn, "filter-chip");
    g_object_set_data(G_OBJECT(btn), "dash", dash);
    g_object_set_data(G_OBJECT(btn), "filter_kind", (gpointer)filter_kind);
    g_object_set_data(G_OBJECT(btn), "filter_value", (gpointer)filter_value);
    g_signal_connect(btn, "clicked", G_CALLBACK(on_lb_filter_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(parent), btn, FALSE, FALSE, 4);
    return btn;
}

static gboolean filter_chip_is_active(AuraDashboard *dash, const char *kind, const char *value) {
    if (!dash || !kind || !value) return FALSE;
    if (strcmp(kind, "domain") == 0) {
        if (!value[0]) return !dash->lb_domain_filter || !dash->lb_domain_filter[0];
        return dash->lb_domain_filter && strcmp(dash->lb_domain_filter, value) == 0;
    }
    if (strcmp(kind, "level") == 0) {
        if (!value[0]) return !dash->lb_level_filter || !dash->lb_level_filter[0];
        return dash->lb_level_filter && strcmp(dash->lb_level_filter, value) == 0;
    }
    return FALSE;
}

static void highlight_filter_row(AuraDashboard *dash, GtkWidget *row) {
    if (!dash || !row) return;
    GList *kids = gtk_container_get_children(GTK_CONTAINER(row));
    for (GList *l = kids; l; l = l->next) {
        GtkWidget *c = GTK_WIDGET(l->data);
        const char *fk = (const char *)g_object_get_data(G_OBJECT(c), "filter_kind");
        const char *fv = (const char *)g_object_get_data(G_OBJECT(c), "filter_value");
        if (!fk || !fv) continue;
        if (filter_chip_is_active(dash, fk, fv))
            add_class(c, "filter-active");
        else
            gtk_style_context_remove_class(gtk_widget_get_style_context(c), "filter-active");
    }
    g_list_free(kids);
}

static GtkWidget *make_hero_domain_btn(AuraDashboard *dash, const char *domain,
    const char *title_markup, const char *subtitle_markup) {
    GtkWidget *btn = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(btn), GTK_RELIEF_NONE);
    add_class(btn, "hero-badge");
    gtk_widget_set_size_request(btn, 220, 116);
    GtkWidget *bv = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_margin_top(bv, 10);
    gtk_widget_set_margin_bottom(bv, 10);
    GtkWidget *tl = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(tl), title_markup);
    gtk_label_set_xalign(GTK_LABEL(tl), 0.5f);
    GtkWidget *sl = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(sl), subtitle_markup);
    gtk_label_set_xalign(GTK_LABEL(sl), 0.5f);
    gtk_box_pack_start(GTK_BOX(bv), tl, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bv), sl, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(btn), bv);
    g_object_set_data(G_OBJECT(btn), "domain", (gpointer)domain);
    g_signal_connect(btn, "clicked", G_CALLBACK(on_domain_button_clicked), dash);
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

    dash_add_section(dash->leaderboard_container, "Filtres matiere et niveau");

    GtkWidget *dom_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    make_filter_chip(dash, dom_row, "Toutes matieres", "domain", "");
    make_filter_chip(dash, dom_row, "Architecture", "domain", "Architecture");
    make_filter_chip(dash, dom_row, "Algorithmique", "domain", "Algorithmique");
    make_filter_chip(dash, dom_row, "Prog. C", "domain", "Programmation C");
    gtk_box_pack_start(GTK_BOX(dash->leaderboard_container), dom_row, FALSE, FALSE, 4);
    highlight_filter_row(dash, dom_row);
    gtk_widget_show_all(dom_row);

    GtkWidget *lvl_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    make_filter_chip(dash, lvl_row, "Tous niveaux", "level", "");
    make_filter_chip(dash, lvl_row, "Junior", "level", "Junior");
    make_filter_chip(dash, lvl_row, "Intermediate", "level", "Intermediate");
    make_filter_chip(dash, lvl_row, "Advanced", "level", "Advanced");
    make_filter_chip(dash, lvl_row, "Expert", "level", "Expert");
    gtk_box_pack_start(GTK_BOX(dash->leaderboard_container), lvl_row, FALSE, FALSE, 8);
    highlight_filter_row(dash, lvl_row);
    gtk_widget_show_all(lvl_row);

    char filt_info[256];
    snprintf(filt_info, sizeof(filt_info), "Actif: %s | %s",
        dash->lb_domain_filter && dash->lb_domain_filter[0] ? dash->lb_domain_filter : "Toutes matieres",
        dash->lb_level_filter && dash->lb_level_filter[0] ? dash->lb_level_filter : "Tous niveaux");
    dash_add_line(dash->leaderboard_container, filt_info, "panel-copy");
    dash_add_section(dash->leaderboard_container,
        "Rang | Etudiant | Moyenne | Meilleur | Entretiens | Matiere");

    DbLeaderboardRow rows[20];
    int n = db_fetch_leaderboard_pro(rows, 20,
        dash->lb_domain_filter, dash->lb_level_filter);

    if (n == 0) {
        dash_add_line(dash->leaderboard_container,
            "Classement vide — passez un entretien pour apparaitre ici.", "panel-copy");
        dash_page_show(dash->leaderboard_container);
        return;
    }
    for (int i = 0; i < n; i++) {
        char line[384];
        snprintf(line, sizeof(line), "#%-2d  %-18s  %3d/10  %3d/10  %3d sess.  %s",
            rows[i].rank, rows[i].user_name, rows[i].avg_score,
            rows[i].best_score, rows[i].interviews, rows[i].domain);
        GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        add_class(row_box, i == 0 ? "lb-row lb-row-gold" : "lb-row");
        gtk_widget_set_margin_start(row_box, 4);
        gtk_widget_set_margin_end(row_box, 4);
        GtkWidget *lbl = gtk_label_new(line);
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.0f);
        add_class(lbl, i == 0 ? "leaderboard-gold" : "panel-copy");
        gtk_box_pack_start(GTK_BOX(row_box), lbl, TRUE, TRUE, 8);
        gtk_box_pack_start(GTK_BOX(dash->leaderboard_container), row_box, FALSE, FALSE, 2);
        gtk_widget_show_all(row_box);
    }
    dash_page_show(dash->leaderboard_container);
}

static void refresh_profile_page(AuraDashboard *dash) {
    if (!dash || !dash->profile_container) return;
    clear_container_children(dash->profile_container);
    Statistics stats = stats_get_user_stats(dash->username);
    char buf[512];

    GtkWidget *hero = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    add_class(hero, "profile-hero");
    GtkWidget *name_lbl = gtk_label_new(NULL);
    snprintf(buf, sizeof(buf),
        "<span size='16000' weight='900' foreground='#FAF7F2'>%s</span>",
        g_aura_session.fullname[0] ? g_aura_session.fullname : "Etudiant");
    gtk_label_set_markup(GTK_LABEL(name_lbl), buf);
    gtk_label_set_xalign(GTK_LABEL(name_lbl), 0.0f);
    gtk_box_pack_start(GTK_BOX(hero), name_lbl, FALSE, FALSE, 0);
    GtkWidget *email_lbl = gtk_label_new(dash->username ? dash->username : "—");
    gtk_label_set_xalign(GTK_LABEL(email_lbl), 0.0f);
    add_class(email_lbl, "user-panel-email");
    gtk_box_pack_start(GTK_BOX(hero), email_lbl, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(dash->profile_container), hero, FALSE, FALSE, 12);
    gtk_widget_show_all(hero);

    GtkWidget *pgrid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(pgrid), 12);
    gtk_grid_set_row_spacing(GTK_GRID(pgrid), 12);
    const char *plabels[] = {"Entretiens passes", "Score moyen"};
    char pvals[2][32];
    snprintf(pvals[0], sizeof(pvals[0]), "%d", stats.interviews_taken);
    snprintf(pvals[1], sizeof(pvals[1]), "%.1f", stats.average_score);
    for (int i = 0; i < 2; i++) {
        GtkWidget *pc = gtk_event_box_new();
        add_class(pc, "glass-card");
        add_class(pc, "profile-mini-stat");
        GtkWidget *pv = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
        gtk_container_set_border_width(GTK_CONTAINER(pv), 16);
        GtkWidget *val = gtk_label_new(pvals[i]);
        gtk_label_set_xalign(GTK_LABEL(val), 0.5f);
        add_class(val, "stat-value");
        GtkWidget *lab = gtk_label_new(plabels[i]);
        gtk_label_set_xalign(GTK_LABEL(lab), 0.5f);
        add_class(lab, "stat-label");
        gtk_box_pack_start(GTK_BOX(pv), val, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(pv), lab, FALSE, FALSE, 0);
        gtk_container_add(GTK_CONTAINER(pc), pv);
        gtk_grid_attach(GTK_GRID(pgrid), pc, i, 0, 1, 1);
    }
    gtk_box_pack_start(GTK_BOX(dash->profile_container), pgrid, FALSE, FALSE, 16);
    gtk_widget_show_all(pgrid);

    dash_add_section(dash->profile_container, "Details du compte");
    snprintf(buf, sizeof(buf), "Matieres: Architecture, Algorithmique, Programmation C");
    dash_add_line(dash->profile_container, buf, "panel-copy");
    dash_page_show(dash->profile_container);
}

static void refresh_settings_page(AuraDashboard *dash) {
    if (!dash || !dash->settings_container) return;
    clear_container_children(dash->settings_container);
    const char *key = aura_config_get_string("groq_api_key", "");
    gboolean has_key = (key && key[0] != '\0') || (getenv("AURA_API_KEY") && getenv("AURA_API_KEY")[0]);
    dash_add_section(dash->settings_container, "Configuration AURA");
    dash_add_line(dash->settings_container,
        has_key ? "Cle API Groq: configuree" : "Cle API Groq: manquante (config/aura.cfg)",
        "panel-copy");
    char dbfile[512], dbline[600];
    aura_fs_get_data_path("local.db", dbfile, sizeof(dbfile));
    snprintf(dbline, sizeof(dbline), "Base SQLite: %s", dbfile);
    dash_add_line(dash->settings_container, dbline, "panel-copy");
    dash_add_line(dash->settings_container,
        "Matieres: Architecture des ordinateurs, Algorithmique, Programmation C", "panel-copy");
    char qbuf[256];
    snprintf(qbuf, sizeof(qbuf),
        "Banque Junior — Archi: %d | Algo: %d | C: %d",
        qb_count("Architecture des ordinateurs", "Junior"),
        qb_count("Algorithmique", "Junior"),
        qb_count("Programmation C", "Junior"));
    dash_add_line(dash->settings_container, qbuf, "panel-copy");
    dash_page_show(dash->settings_container);
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

    char dbgpath[1024];
    FILE *dbg = aura_fs_get_data_path("login_screen_debug.log", dbgpath, sizeof(dbgpath))
        ? fopen(dbgpath, "a") : NULL;
    if (dbg != NULL) {
        fprintf(dbg, "[DASHBOARD] nav clicked route=%s\n", route != NULL ? route : "(null)");
        fclose(dbg);
    }

    if (dash == NULL || route == NULL) {
        return;
    }

    if (!dash->interactions_enabled) {
        FILE *guard = aura_fs_get_data_path("login_screen_debug.log", dbgpath, sizeof(dbgpath))
            ? fopen(dbgpath, "a") : NULL;
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

    switch_dashboard_page(dash, route, widget);

    if (g_strcmp0(route, "reports") == 0) refresh_reports_page(dash);
    else if (g_strcmp0(route, "leaderboard") == 0) refresh_leaderboard_page(dash);
    else if (g_strcmp0(route, "profile") == 0) refresh_profile_page(dash);
    else if (g_strcmp0(route, "settings") == 0) refresh_settings_page(dash);
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
    (void)user_data;
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
    (void)user_data;
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
    (void)user_data;
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
    bool has_report = report_generate_session_pdf(iv, avg, pdf_path, sizeof(pdf_path));
    bool has_pdf = has_report && g_str_has_suffix(pdf_path, ".pdf");

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
                "Rapport PDF genere.", avg);
        } else if (has_report) {
            snprintf(msg, sizeof(msg),
                "Entretien termine !\nScore moyen: %d/10\n"
                "Resultats enregistres dans SQLite.\n"
                "Rapport texte genere (fallback).", avg);
        } else {
            snprintf(msg, sizeof(msg),
                "Entretien termine !\nScore moyen: %d/10\n"
                "Resultats enregistres dans SQLite.\n"
                "(Rapport non genere — voir data/reports/)", avg);
        }
        GtkWidget *result = gtk_message_dialog_new(GTK_WINDOW(dash->window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_INFO,
            has_report ? GTK_BUTTONS_YES_NO : GTK_BUTTONS_OK,
            "%s", msg);
        if (has_report) {
            gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(result),
                "Fichier: %s\n\nOuvrir le rapport maintenant ?", pdf_path);
        } else {
            char reports_dir[1024];
            if (aura_fs_get_data_path("reports", reports_dir, sizeof(reports_dir)))
                gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(result),
                    "Dossier rapports: %s", reports_dir);
        }
        gint resp = gtk_dialog_run(GTK_DIALOG(result));
        gtk_widget_destroy(result);
        if (has_report && resp == GTK_RESPONSE_YES)
            report_open_file(pdf_path);
    }
}

static void interview_update_progress(GtkWidget *dlg, Interview *iv) {
    GtkWidget *progress = GTK_WIDGET(g_object_get_data(G_OBJECT(dlg), "progress_label"));
    GtkWidget *pbar = GTK_WIDGET(g_object_get_data(G_OBJECT(dlg), "progress_bar"));
    if (!iv) return;
    int idx = interview_get_current_index(iv);
    int total = interview_get_questions_count(iv);
    char buf[64];
    snprintf(buf, sizeof(buf), "Question %d sur %d", idx + 1, total);
    if (progress) gtk_label_set_text(GTK_LABEL(progress), buf);
    if (pbar && total > 0) {
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pbar), (gdouble)(idx + 1) / (gdouble)total);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(pbar), buf);
    }
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
    gtk_window_set_default_size(GTK_WINDOW(dlg), 860, 640);

    GtkWidget *outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_set_border_width(GTK_CONTAINER(outer), 18);
    gtk_container_add(GTK_CONTAINER(dlg), outer);

    GtkWidget *main_card = gtk_event_box_new();
    add_class(main_card, "glass-card");
    add_class(main_card, "interview-card");
    gtk_box_pack_start(GTK_BOX(outer), main_card, TRUE, TRUE, 0);
    GtkWidget *v = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(v), 22);
    gtk_container_add(GTK_CONTAINER(main_card), v);

    char header[256];
    snprintf(header, sizeof(header), "%s — %s",
        dash->selected_domain ? dash->selected_domain : "Algorithmique",
        dash->selected_level ? dash->selected_level : "Junior");
    GtkWidget *header_label = gtk_label_new(header);
    gtk_label_set_xalign(GTK_LABEL(header_label), 0.0f);
    add_class(header_label, "panel-heading");
    gtk_box_pack_start(GTK_BOX(v), header_label, FALSE, FALSE, 0);

    GtkWidget *progress_label = gtk_label_new("Question 1 sur 5");
    gtk_label_set_xalign(GTK_LABEL(progress_label), 0.0f);
    add_class(progress_label, "dash-accent");
    gtk_box_pack_start(GTK_BOX(v), progress_label, FALSE, FALSE, 2);
    g_object_set_data(G_OBJECT(dlg), "progress_label", progress_label);

    GtkWidget *progress_bar = gtk_progress_bar_new();
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0.2);
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(progress_bar), TRUE);
    gtk_widget_set_size_request(progress_bar, -1, 14);
    gtk_box_pack_start(GTK_BOX(v), progress_bar, FALSE, FALSE, 8);
    g_object_set_data(G_OBJECT(dlg), "progress_bar", progress_bar);

    GtkWidget *q_frame = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    add_class(q_frame, "glass-card");
    gtk_container_set_border_width(GTK_CONTAINER(q_frame), 16);
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
    gtk_window_set_title(GTK_WINDOW(dlg), "Etape 1 — Matiere");
    gtk_window_set_default_size(GTK_WINDOW(dlg), 860, 420);

    GtkWidget *outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_set_border_width(GTK_CONTAINER(outer), 20);
    gtk_container_add(GTK_CONTAINER(dlg), outer);

    GtkWidget *card = gtk_event_box_new();
    add_class(card, "glass-card");
    add_class(card, "picker-card");
    gtk_box_pack_start(GTK_BOX(outer), card, TRUE, TRUE, 0);
    GtkWidget *v = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    gtk_container_set_border_width(GTK_CONTAINER(v), 28);
    gtk_container_add(GTK_CONTAINER(card), v);

    GtkWidget *step = gtk_label_new("ETAPE 1 / 2");
    gtk_label_set_xalign(GTK_LABEL(step), 0.0f);
    add_class(step, "step-pill");
    gtk_box_pack_start(GTK_BOX(v), step, FALSE, FALSE, 0);

    GtkWidget *title = gtk_label_new("Choisissez votre matiere");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0f);
    add_class(title, "panel-heading");
    gtk_box_pack_start(GTK_BOX(v), title, FALSE, FALSE, 0);

    GtkWidget *sub = gtk_label_new("L'IA Groq generera des questions adaptees a votre choix.");
    gtk_label_set_xalign(GTK_LABEL(sub), 0.0f);
    add_class(sub, "panel-copy");
    gtk_box_pack_start(GTK_BOX(v), sub, FALSE, FALSE, 8);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 14);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 14);
    gtk_box_pack_start(GTK_BOX(v), grid, TRUE, TRUE, 12);

    const gchar *domains[] = {
        "Architecture des ordinateurs",
        "Algorithmique",
        "Programmation C"
    };
    const gchar *titles[] = {
        "<span weight='800' size='12000'>Architecture</span>",
        "<span weight='800' size='12000'>Algorithmique</span>",
        "<span weight='800' size='12000'>Programmation C</span>"
    };
    const gchar *subs[] = {
        "<span size='9000' foreground='#9A8FAE'>CPU, memoire, bus</span>",
        "<span size='9000' foreground='#9A8FAE'>Complexite, graphes</span>",
        "<span size='9000' foreground='#9A8FAE'>Pointeurs, malloc</span>"
    };
    for (int i = 0; i < 3; ++i) {
        GtkWidget *btn = make_hero_domain_btn(dash, domains[i], titles[i], subs[i]);
        gtk_grid_attach(GTK_GRID(grid), btn, i, 0, 1, 1);
        if (i == 1) {
            if (dash->selected_domain) g_free(dash->selected_domain);
            dash->selected_domain = g_strdup(domains[i]);
            add_class(btn, "active");
        }
    }

    GtkWidget *btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *btn_continue = gtk_button_new_with_label("Continuer vers le niveau");
    gtk_button_set_relief(GTK_BUTTON(btn_continue), GTK_RELIEF_NONE);
    add_class(btn_continue, "primary-action");
    gtk_widget_set_halign(btn_continue, GTK_ALIGN_END);
    gtk_box_pack_end(GTK_BOX(btn_row), btn_continue, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(v), btn_row, FALSE, FALSE, 0);

    g_object_set_data(G_OBJECT(dlg), "dash", dash);
    g_signal_connect(btn_continue, "clicked", G_CALLBACK(on_domain_continue_clicked), dlg);
    gtk_widget_show_all(dlg);
}

static void level_dialog_show(AuraDashboard *dash) {
    if (!dash) return;
    GtkWidget *dlg = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    add_class(dlg, "picker-window");
    gtk_window_set_transient_for(GTK_WINDOW(dlg), GTK_WINDOW(dash->window));
    gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
    gtk_window_set_title(GTK_WINDOW(dlg), "Etape 2 — Niveau");
    gtk_window_set_default_size(GTK_WINDOW(dlg), 820, 400);

    GtkWidget *outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_set_border_width(GTK_CONTAINER(outer), 20);
    gtk_container_add(GTK_CONTAINER(dlg), outer);

    GtkWidget *card = gtk_event_box_new();
    add_class(card, "glass-card");
    add_class(card, "picker-card");
    gtk_box_pack_start(GTK_BOX(outer), card, TRUE, TRUE, 0);
    GtkWidget *v = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    gtk_container_set_border_width(GTK_CONTAINER(v), 28);
    gtk_container_add(GTK_CONTAINER(card), v);

    GtkWidget *step = gtk_label_new("ETAPE 2 / 2");
    gtk_label_set_xalign(GTK_LABEL(step), 0.0f);
    add_class(step, "step-pill");
    gtk_box_pack_start(GTK_BOX(v), step, FALSE, FALSE, 0);

    GtkWidget *level_title = gtk_label_new("Choisissez la difficulte");
    gtk_label_set_xalign(GTK_LABEL(level_title), 0.0f);
    add_class(level_title, "panel-heading");
    gtk_box_pack_start(GTK_BOX(v), level_title, FALSE, FALSE, 0);

    GtkWidget *level_sub = gtk_label_new("5 questions — correction IA en temps reel.");
    gtk_label_set_xalign(GTK_LABEL(level_sub), 0.0f);
    add_class(level_sub, "panel-copy");
    gtk_box_pack_start(GTK_BOX(v), level_sub, FALSE, FALSE, 8);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_box_pack_start(GTK_BOX(v), grid, TRUE, TRUE, 0);

    const gchar *levels[] = {"Junior", "Intermediate", "Advanced", "Expert"};
    const gchar *level_labels[] = {"Junior", "Intermediaire", "Avance", "Expert"};
    for (int i = 0; i < 4; ++i) {
        GtkWidget *btn = gtk_button_new_with_label(level_labels[i]);
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
    GtkWidget *btn_continue = gtk_button_new_with_label("Demarrer l'entretien");
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
        AURA_CSS_BASE
        "window.dashboard-window, window.interview-window, window.picker-window {"
        "  background-color: #0A0812; color: #FAF7F2;"
        "}\n"
        "label { color: #D8CEE8; }\n"
        "entry { background-color: rgba(20, 16, 31, 0.95); color: #FAF7F2; border: 1px solid rgba(139, 124, 246, 0.22); border-radius: 10px; }\n"
        "textview { background-color: rgba(20, 16, 31, 0.95); color: #FAF7F2; }\n"
        "textview text { background-color: transparent; color: #FAF7F2; }\n"
        ".dashboard-root { background: linear-gradient(145deg, #0A0812 0%, #14101F 42%, #0E0C16 100%); }\n"
        ".sidebar {"
        "  background: linear-gradient(180deg, #14101F 0%, #0E0C16 100%);"
        "  border-right: 1px solid rgba(240, 180, 41, 0.12);"
        "  padding: 24px 16px;"
        "  box-shadow: 6px 0 32px rgba(0, 0, 0, 0.35);"
        "}\n"
        ".sidebar-brand { color: #FAF7F2; font-weight: 900; letter-spacing: 1px; margin-bottom: 4px; }\n"
        ".sidebar-tagline { color: #6E6478; font-size: 11px; font-weight: 600; letter-spacing: 2px; margin-bottom: 20px; }\n"
        "button.nav-button, .nav-button {"
        "  background-color: rgba(30, 24, 40, 0.7);"
        "  color: #B8AEC8;"
        "  border: 1px solid rgba(139, 124, 246, 0.12);"
        "  border-radius: 10px;"
        "  padding: 14px 18px;"
        "  font-size: 13px;"
        "  font-weight: 600;"
        "}\n"
        "button.nav-button label, .nav-button label { color: #B8AEC8; }\n"
        ".nav-button:hover { background-color: rgba(139, 124, 246, 0.18); border-color: rgba(139, 124, 246, 0.35); }\n"
        ".nav-button:hover label { color: #FAF7F2; }\n"
        ".nav-button.active {"
        "  background: linear-gradient(135deg, rgba(240, 180, 41, 0.22) 0%, rgba(139, 124, 246, 0.16) 100%);"
        "  border-color: rgba(240, 180, 41, 0.45);"
        "  box-shadow: 0 4px 20px rgba(240, 180, 41, 0.12);"
        "}\n"
        ".nav-button.active label { color: #FFF8EC; font-weight: 700; }\n"
        "button.primary-action, .primary-action {"
        "  background-image: linear-gradient(135deg, #F0B429 0%, #C4922A 100%);"
        "  background-color: #F0B429;"
        "  color: #1A1208;"
        "  border: none;"
        "  border-radius: 12px;"
        "  padding: 16px 32px;"
        "  font-size: 15px;"
        "  font-weight: 800;"
        "  box-shadow: 0 8px 28px rgba(240, 180, 41, 0.32);"
        "}\n"
        "button.primary-action label, .primary-action label { color: #1A1208; font-weight: 800; }\n"
        ".primary-action:hover { background-image: linear-gradient(135deg, #F8C84A 0%, #F0B429 100%); }\n"
        ".hero-cta { padding: 18px 36px; font-size: 16px; }\n"
        "button.secondary-action, .secondary-action {"
        "  background-color: rgba(30, 24, 40, 0.85);"
        "  color: #C4B8D8;"
        "  border: 1px solid rgba(139, 124, 246, 0.2);"
        "  border-radius: 10px;"
        "  padding: 12px 18px;"
        "  font-size: 13px;"
        "  font-weight: 600;"
        "}\n"
        "button.secondary-action label, .secondary-action label { color: #C4B8D8; }\n"
        ".secondary-action:hover { background-color: rgba(139, 124, 246, 0.2); border-color: rgba(139, 124, 246, 0.4); }\n"
        "button.hero-badge, .hero-badge {"
        "  background-color: rgba(30, 24, 40, 0.9);"
        "  border: 2px solid rgba(139, 124, 246, 0.28);"
        "  border-radius: 14px;"
        "  padding: 18px;"
        "  color: #FAF7F2;"
        "}\n"
        "button.hero-badge label, .hero-badge label { color: #FAF7F2; font-weight: 700; }\n"
        ".hero-badge:hover { background-color: rgba(139, 124, 246, 0.2); border-color: rgba(139, 124, 246, 0.5); }\n"
        ".hero-badge.active { background-color: rgba(240, 180, 41, 0.18); border-color: rgba(240, 180, 41, 0.6); }\n"
        ".hero-badge.active label { color: #FFF8EC; }\n"
        ".glass-card {"
        "  background-color: rgba(30, 24, 40, 0.75);"
        "  border: 1px solid rgba(139, 124, 246, 0.14);"
        "  border-radius: 18px;"
        "  box-shadow: 0 12px 40px rgba(0, 0, 0, 0.28);"
        "}\n"
        ".stat-card { padding: 4px; }\n"
        ".stat-value { color: #F0B429; font-size: 32px; font-weight: 900; }\n"
        ".stat-label { color: #6E6478; font-size: 11px; font-weight: 700; letter-spacing: 1.2px; }\n"
        ".panel-heading { color: #FAF7F2; font-size: 28px; font-weight: 900; letter-spacing: -0.5px; }\n"
        ".panel-copy { color: #9A8FAE; font-size: 14px; }\n"
        ".section-title { color: #8B7CF6; font-size: 12px; font-weight: 800; letter-spacing: 1.5px; margin-top: 8px; }\n"
        "separator.section-sep { background-color: rgba(139, 124, 246, 0.15); min-height: 1px; }\n"
        ".welcome-hero { color: #FAF7F2; font-size: 18px; font-weight: 700; }\n"
        ".dash-muted { color: #9A8FAE; font-size: 13px; }\n"
        ".dash-accent { color: #8B7CF6; font-size: 13px; font-weight: 600; }\n"
        ".dash-footer { color: #5A5068; font-size: 11px; letter-spacing: 0.3px; }\n"
        ".ai-pill {"
        "  background-color: rgba(52, 211, 153, 0.12);"
        "  color: #34D399;"
        "  border: 1px solid rgba(52, 211, 153, 0.25);"
        "  border-radius: 999px;"
        "  padding: 6px 14px;"
        "  font-size: 12px;"
        "  font-weight: 700;"
        "}\n"
        ".report-row {"
        "  background-color: rgba(20, 16, 31, 0.6);"
        "  border: 1px solid rgba(139, 124, 246, 0.1);"
        "  border-radius: 10px;"
        "  padding: 10px 14px;"
        "}\n"
        ".report-filename { color: #D8CEE8; font-size: 13px; font-weight: 600; }\n"
        ".lb-row {"
        "  background-color: rgba(20, 16, 31, 0.45);"
        "  border-radius: 8px;"
        "  padding: 8px 12px;"
        "}\n"
        ".lb-row-gold {"
        "  background: linear-gradient(90deg, rgba(240, 180, 41, 0.12) 0%, rgba(20, 16, 31, 0.4) 100%);"
        "  border: 1px solid rgba(240, 180, 41, 0.25);"
        "}\n"
        ".leaderboard-gold { color: #F0B429; font-weight: 800; font-size: 14px; }\n"
        ".answer-scroll { background-color: rgba(20, 16, 31, 0.95); border: 2px solid rgba(139, 124, 246, 0.25); border-radius: 12px; }\n"
        ".question-card { color: #FAF7F2; font-size: 15px; font-weight: 600; padding: 8px; }\n"
        ".answer-box { color: #FAF7F2; font-size: 14px; }\n"
        ".verdict-badge { font-size: 16px; font-weight: 900; padding: 6px 16px; border-radius: 999px; }\n"
        ".verdict-true { color: #34D399; background: rgba(52, 211, 153, 0.15); }\n"
        ".verdict-false { color: #F87171; background: rgba(248, 113, 113, 0.15); }\n"
        ".verdict-partial { color: #FBBF24; background: rgba(251, 191, 36, 0.15); }\n"
        ".feedback-detail { color: #9A8FAE; font-size: 13px; }\n"
        ".dash-scroll { background-color: transparent; border: none; }\n"
        ".dash-scroll viewport { background-color: transparent; }\n"
        "scrolledwindow, scrolledwindow viewport { background-color: transparent; }\n"
        ".mini-card-title { color: #8B7CF6; font-size: 11px; font-weight: 700; letter-spacing: 1px; }\n"
        ".user-panel-name { color: #FAF7F2; font-size: 15px; font-weight: 700; }\n"
        ".user-panel-email { color: #9A8FAE; font-size: 13px; }\n"
        ".step-pill {"
        "  color: #F0B429;"
        "  font-size: 11px;"
        "  font-weight: 800;"
        "  letter-spacing: 2px;"
        "  background-color: rgba(240, 180, 41, 0.1);"
        "  border: 1px solid rgba(240, 180, 41, 0.25);"
        "  border-radius: 999px;"
        "  padding: 6px 14px;"
        "}\n"
        ".picker-card { box-shadow: 0 20px 60px rgba(0, 0, 0, 0.4); }\n"
        ".interview-card { min-height: 520px; }\n"
        ".filter-chip { border-radius: 999px; padding: 8px 14px; }\n"
        "button.secondary-action.filter-chip.filter-active,"
        ".secondary-action.filter-chip.filter-active {"
        "  background-color: rgba(240, 180, 41, 0.18);"
        "  border-color: rgba(240, 180, 41, 0.5);"
        "  color: #FFF8EC;"
        "}\n"
        "button.secondary-action.filter-chip.filter-active label,"
        ".secondary-action.filter-chip.filter-active label {"
        "  color: #FFF8EC;"
        "  font-weight: 700;"
        "}\n"
        ".profile-hero {"
        "  background-color: rgba(20, 16, 31, 0.5);"
        "  border: 1px solid rgba(139, 124, 246, 0.15);"
        "  border-radius: 14px;"
        "  padding: 18px 20px;"
        "  margin: 4px 8px;"
        "}\n"
        ".profile-mini-stat { min-width: 140px; }\n"
        ".stat-accent-0 { border-top: 3px solid #F0B429; }\n"
        ".stat-accent-1 { border-top: 3px solid #8B7CF6; }\n"
        ".stat-accent-2 { border-top: 3px solid #34D399; }\n"
        ".stat-accent-3 { border-top: 3px solid #F87171; }\n"
        ".tip-card {"
        "  background-color: rgba(20, 16, 31, 0.55);"
        "  border: 1px solid rgba(139, 124, 246, 0.12);"
        "  border-radius: 12px;"
        "  padding: 14px 16px;"
        "}\n"
        ".tip-title { color: #F0B429; font-size: 12px; font-weight: 800; letter-spacing: 0.5px; }\n"
        ".tip-copy { color: #9A8FAE; font-size: 12px; }\n"
        ".score-high { color: #34D399; font-weight: 700; }\n"
        ".score-mid { color: #FBBF24; font-weight: 700; }\n"
        ".score-low { color: #F87171; font-weight: 700; }\n";

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
    gtk_widget_set_size_request(dash->sidebar, 248, -1);
    gtk_box_pack_start(GTK_BOX(root), dash->sidebar, FALSE, FALSE, 0);

    GtkWidget *brand = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(brand),
        "<span size='16000' weight='900' foreground='#F0B429'>ESISA</span>\n"
        "<span size='13000' weight='800' foreground='#FAF7F2'>AURA</span>");
    gtk_label_set_justify(GTK_LABEL(brand), GTK_JUSTIFY_LEFT);
    gtk_label_set_xalign(GTK_LABEL(brand), 0.0f);
    add_class(brand, "sidebar-brand");
    gtk_box_pack_start(GTK_BOX(dash->sidebar), brand, FALSE, FALSE, 0);

    GtkWidget *tagline = gtk_label_new("SIMULATEUR D'ENTRETIENS");
    gtk_label_set_xalign(GTK_LABEL(tagline), 0.0f);
    add_class(tagline, "sidebar-tagline");
    gtk_box_pack_start(GTK_BOX(dash->sidebar), tagline, FALSE, FALSE, 0);

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

    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_widget_set_margin_start(content, 4);
    gtk_widget_set_margin_end(content, 4);
    gtk_box_pack_start(GTK_BOX(overview_page), content, TRUE, TRUE, 0);

    GtkWidget *top_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
    gtk_box_pack_start(GTK_BOX(content), top_row, FALSE, FALSE, 0);

    GtkWidget *welcome_col = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_box_pack_start(GTK_BOX(top_row), welcome_col, TRUE, TRUE, 0);

    dash->welcome_label = gtk_label_new("Bienvenue — simulateur d'entretiens ESISA");
    gtk_label_set_xalign(GTK_LABEL(dash->welcome_label), 0.0f);
    add_class(dash->welcome_label, "welcome-hero");
    gtk_box_pack_start(GTK_BOX(welcome_col), dash->welcome_label, FALSE, FALSE, 0);

    GtkWidget *welcome_sub = gtk_label_new("Preparez vos entretiens techniques avec l'IA Groq");
    gtk_label_set_xalign(GTK_LABEL(welcome_sub), 0.0f);
    add_class(welcome_sub, "dash-muted");
    gtk_box_pack_start(GTK_BOX(welcome_col), welcome_sub, FALSE, FALSE, 0);

    dash->ai_status_label = gtk_label_new("IA Groq: prete");
    add_class(dash->ai_status_label, "ai-pill");
    gtk_widget_set_halign(dash->ai_status_label, GTK_ALIGN_END);
    gtk_widget_set_valign(dash->ai_status_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(top_row), dash->ai_status_label, FALSE, FALSE, 0);

    GtkWidget *launch_interview_btn = gtk_button_new_with_label("  Lancer un entretien  ");
    gtk_button_set_relief(GTK_BUTTON(launch_interview_btn), GTK_RELIEF_NONE);
    add_class(launch_interview_btn, "primary-action");
    add_class(launch_interview_btn, "hero-cta");
    gtk_widget_set_halign(launch_interview_btn, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(content), launch_interview_btn, FALSE, FALSE, 4);
    g_signal_connect(launch_interview_btn, "clicked", G_CALLBACK(launch_interview_handler), dash);

    // Stats cards
    GtkWidget *cards = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(cards), 12);
    gtk_grid_set_column_spacing(GTK_GRID(cards), 12);
    gtk_box_pack_start(GTK_BOX(content), cards, FALSE, FALSE, 0);

    const gchar *stat_labels[] = {"Entretiens", "Moyenne /10", "Total global", "Session (min)"};
    const gchar *stat_accents[] = {"stat-accent-0", "stat-accent-1", "stat-accent-2", "stat-accent-3"};
    for (int i = 0; i < 4; i++) {
        GtkWidget *card = gtk_event_box_new();
        add_class(card, "glass-card");
        add_class(card, "stat-card");
        add_class(card, stat_accents[i]);
        GtkWidget *v = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_container_set_border_width(GTK_CONTAINER(v), 22);
        gtk_widget_set_halign(v, GTK_ALIGN_CENTER);
        GtkWidget *value = gtk_label_new("0");
        gtk_label_set_xalign(GTK_LABEL(value), 0.5f);
        add_class(value, "stat-value");
        GtkWidget *label = gtk_label_new(stat_labels[i]);
        gtk_label_set_xalign(GTK_LABEL(label), 0.5f);
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

    GtkWidget *progress_card = gtk_event_box_new();
    add_class(progress_card, "glass-card");
    GtkWidget *pbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(pbox), 18);
    GtkWidget *ptitle = gtk_label_new("PROGRESSION");
    gtk_label_set_xalign(GTK_LABEL(ptitle), 0.0f);
    add_class(ptitle, "mini-card-title");
    gtk_box_pack_start(GTK_BOX(pbox), ptitle, FALSE, FALSE, 0);
    dash->progress_bar = gtk_progress_bar_new();
    gtk_widget_set_size_request(dash->progress_bar, -1, 12);
    gtk_box_pack_start(GTK_BOX(pbox), dash->progress_bar, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(progress_card), pbox);
    gtk_box_pack_start(GTK_BOX(left_col), progress_card, FALSE, FALSE, 0);

    GtkWidget *counters_card = gtk_event_box_new();
    add_class(counters_card, "glass-card");
    GtkWidget *cb = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(cb), 18);
    GtkWidget *ctitle = gtk_label_new("ACTIVITE");
    gtk_label_set_xalign(GTK_LABEL(ctitle), 0.0f);
    add_class(ctitle, "mini-card-title");
    gtk_box_pack_start(GTK_BOX(cb), ctitle, FALSE, FALSE, 0);
    dash->session_counter_label = gtk_label_new("Entretiens: 0");
    gtk_label_set_xalign(GTK_LABEL(dash->session_counter_label), 0.0f);
    add_class(dash->session_counter_label, "panel-copy");
    gtk_box_pack_start(GTK_BOX(cb), dash->session_counter_label, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(counters_card), cb);
    gtk_box_pack_start(GTK_BOX(left_col), counters_card, FALSE, FALSE, 0);

    GtkWidget *user_panel = gtk_event_box_new();
    add_class(user_panel, "glass-card");
    GtkWidget *up = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(up), 20);
    GtkWidget *utitle = gtk_label_new("MON PROFIL");
    gtk_label_set_xalign(GTK_LABEL(utitle), 0.0f);
    add_class(utitle, "mini-card-title");
    gtk_box_pack_start(GTK_BOX(up), utitle, FALSE, FALSE, 0);
    char uname_buf[192];
    snprintf(uname_buf, sizeof(uname_buf), "%s",
        g_aura_session.fullname[0] ? g_aura_session.fullname : dash->username);
    GtkWidget *uname = gtk_label_new(uname_buf);
    gtk_label_set_xalign(GTK_LABEL(uname), 0.0f);
    add_class(uname, "user-panel-name");
    gtk_box_pack_start(GTK_BOX(up), uname, FALSE, FALSE, 0);
    char email_buf[192];
    snprintf(email_buf, sizeof(email_buf), "%s", dash->username ? dash->username : "—");
    GtkWidget *uemail = gtk_label_new(email_buf);
    gtk_label_set_xalign(GTK_LABEL(uemail), 0.0f);
    add_class(uemail, "user-panel-email");
    gtk_box_pack_start(GTK_BOX(up), uemail, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(user_panel), up);
    gtk_box_pack_start(GTK_BOX(mid), user_panel, FALSE, FALSE, 0);

    GtkWidget *tips = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(tips), 12);
    gtk_widget_set_margin_top(tips, 8);
    const char *tip_titles[] = {"IA Groq", "5 questions", "Rapport PDF"};
    const char *tip_texts[] = {
        "Correction intelligente en francais",
        "Parcours guide question par question",
        "Genere automatiquement en fin de session"
    };
    for (int i = 0; i < 3; i++) {
        GtkWidget *tc = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
        add_class(tc, "tip-card");
        gtk_widget_set_margin_bottom(tc, 4);
        GtkWidget *tt = gtk_label_new(tip_titles[i]);
        gtk_label_set_xalign(GTK_LABEL(tt), 0.0f);
        add_class(tt, "tip-title");
        GtkWidget *tx = gtk_label_new(tip_texts[i]);
        gtk_label_set_xalign(GTK_LABEL(tx), 0.0f);
        gtk_label_set_line_wrap(GTK_LABEL(tx), TRUE);
        add_class(tx, "tip-copy");
        gtk_box_pack_start(GTK_BOX(tc), tt, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(tc), tx, FALSE, FALSE, 0);
        gtk_grid_attach(GTK_GRID(tips), tc, i, 0, 1, 1);
    }
    gtk_box_pack_start(GTK_BOX(content), tips, FALSE, FALSE, 0);

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
