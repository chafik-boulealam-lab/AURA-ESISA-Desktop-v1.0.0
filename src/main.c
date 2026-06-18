#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "api.h"
#include "db.h"
#include "dashboard_ui.h"
#include "login_ui.h"
#include "auth.h"
#include "launcher.h"
#include "filesystem.h"
#include "config.h"

// Pointeurs globaux
GtkWidget *text_view;
GtkWidget *entry_nom;
GtkTextBuffer *buffer;

// Variables globales pour le compte rendu
static int global_questions_answered = 0;
static int global_correct_answers = 0;
static int global_incorrect_answers = 0;

// Contexte pour chaque onglet
typedef struct {
    const gchar *category;
    GtkWidget *label_question;
    GtkWidget *entry_reponse;
    GtkWidget *spinner;
    GtkWidget *btn_gen;
    GtkWidget *btn_submit;
} TabContext;

TabContext tabs[3];

static void add_class(GtkWidget *widget, const char *class_name) {
    gtk_style_context_add_class(gtk_widget_get_style_context(widget), class_name);
}

static void apply_aura_css(void) {
    GtkCssProvider *provider = gtk_css_provider_new();
    const gchar *css =
        "* { font-family: 'Segoe UI', 'Inter', 'Roboto', sans-serif; color: #F5F0E8; }"
        "window.aura-app { background-color: #12101A; }"
        ".aura-root { background: linear-gradient(135deg, #12101A 0%, #1A1628 52%, #12101A 100%); }"
        ".hero-card, .panel-card, .log-card {"
        "  background-color: rgba(30, 26, 46, 0.92);"
        "  border: 1px solid rgba(124, 108, 240, 0.2);"
        "  border-radius: 20px;"
        "  box-shadow: 0 16px 48px rgba(0, 0, 0, 0.35);"
        "}"
        ".hero-card { padding: 28px; background: linear-gradient(135deg, rgba(30, 26, 46, 0.95) 0%, rgba(42, 37, 64, 0.9) 100%); }"
        ".aura-kicker { color: #E8A838; font-size: 12px; font-weight: 700; letter-spacing: 3px; }"
        ".aura-title { color: #F5F0E8; font-size: 40px; font-weight: 900; }"
        ".aura-title .accent { color: #E8A838; }"
        ".aura-subtitle { color: #A89BB8; font-size: 15px; }"
        ".chip-row { margin-top: 18px; }"
        ".hero-badge {"
        "  background: rgba(232, 168, 56, 0.12);"
        "  border: 1.5px solid rgba(232, 168, 56, 0.3);"
        "  border-radius: 12px; padding: 10px 16px; color: #E8A838; font-size: 13px; font-weight: 700;"
        "}"
        ".field-label, .panel-label { color: #D4C8E8; font-size: 13px; font-weight: 700; }"
        ".name-entry, .response-entry {"
        "  background-color: rgba(26, 22, 40, 0.95); color: #F5F0E8;"
        "  border: 1.5px solid rgba(124, 108, 240, 0.25); border-radius: 12px; padding: 14px 16px; font-size: 14px;"
        "}"
        ".name-entry:focus, .response-entry:focus {"
        "  border-color: #E8A838; box-shadow: 0 0 0 3px rgba(232, 168, 56, 0.12);"
        "}"
        "button.primary-action, .primary-action {"
        "  background-image: linear-gradient(135deg, #E8A838 0%, #D4922A 100%);"
        "  color: #1A1208; border: none; border-radius: 12px; padding: 14px 22px;"
        "  font-weight: 700; font-size: 14px; box-shadow: 0 8px 20px rgba(232, 168, 56, 0.3);"
        "}"
        ".primary-action:hover { background-image: linear-gradient(135deg, #F0B84A 0%, #E8A838 100%); }"
        "button.secondary-action, .secondary-action {"
        "  background-color: rgba(124, 108, 240, 0.15); color: #D4C8E8;"
        "  border: 1.5px solid rgba(124, 108, 240, 0.3); border-radius: 12px; padding: 12px 20px; font-weight: 700;"
        "}"
        ".secondary-action:hover { background-color: rgba(124, 108, 240, 0.28); color: #FFFFFF; }"
        ".question-card {"
        "  background: rgba(30, 26, 46, 0.95); color: #F5F0E8;"
        "  border-left: 5px solid #E8A838; border-radius: 16px; padding: 22px; font-size: 15px;"
        "}"
        ".action-row { margin-top: 16px; }"
        "notebook { background-color: transparent; }"
        "notebook tab {"
        "  background-color: rgba(42, 37, 64, 0.6); color: #8B7FA0;"
        "  border-radius: 12px 12px 0 0; padding: 12px 20px; margin-right: 6px; font-weight: 700; font-size: 13px;"
        "}"
        "notebook tab:hover { background-color: rgba(124, 108, 240, 0.2); color: #D4C8E8; }"
        "notebook tab:checked {"
        "  background: linear-gradient(135deg, rgba(232, 168, 56, 0.2) 0%, rgba(124, 108, 240, 0.15) 100%);"
        "  color: #E8A838; border-bottom: 3px solid #E8A838;"
        "}"
        ".tab-page { padding: 10px; }"
        ".log-view {"
        "  background-color: rgba(26, 22, 40, 0.95); color: #D4C8E8;"
        "  border-radius: 14px; padding: 16px;"
        "  font-family: 'Cascadia Mono', 'Consolas', monospace; font-size: 13px;"
        "}"
        ".log-scroller { background-color: transparent; }"
        ".panel-heading { color: #F5F0E8; font-size: 20px; font-weight: 900; }"
        ".panel-copy { color: #A89BB8; font-size: 13px; }"
        "textview.log-view text { background-color: transparent; }"
        "scrollbar slider { background-color: rgba(232, 168, 56, 0.3); border-radius: 6px; }"
        "scrollbar slider:hover { background-color: rgba(124, 108, 240, 0.45); }"
    ;

    gtk_css_provider_load_from_data(provider, css, -1, NULL);
    GdkScreen *screen = gdk_screen_get_default();
    if (screen == NULL) {
        fprintf(stderr, "Erreur: aucun écran GTK disponible.\n");
        g_object_unref(provider);
        return;
    }
    gtk_style_context_add_provider_for_screen(
        screen,
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    g_object_unref(provider);
}

static const char *get_local_question(int categorie) {
    static const char *algorithmique[] = {
        "Explique la différence entre une pile et une file, puis donne un cas d'utilisation de chacune.",
        "Comment détecter un cycle dans un graphe orienté ?",
        "Quel est le principe du tri fusion et pourquoi est-il efficace ?"
    };
    static const char *mathematiques[] = {
        "Tu as 8 billes dont une est plus lourde. Comment la trouver en 2 pesées ?",
        "Pourquoi le problème des ponts de Koenigsberg est-il célèbre en algorithmique ?",
        "Quel raisonnement utiliser pour résoudre une énigme de logique avec des interrupteurs et des ampoules ?"
    };
    static const char *culture_it[] = {
        "Qu'est-ce que l'architecture von Neumann et pourquoi elle est fondamentale ?",
        "Quelle différence entre RAM, SSD et cache processeur ?",
        "Comment TCP se distingue-t-il d'UDP dans un contexte réseau ?"
    };

    switch (categorie) {
        case 1:
            return algorithmique[rand() % 3];
        case 2:
            return mathematiques[rand() % 3];
        case 3:
            return culture_it[rand() % 3];
        default:
            return "Propose une question d'entretien technique simple et claire.";
    }
}

// Logger
void append_log(const gchar *text) {
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(buffer, &iter);
    gtk_text_buffer_insert(buffer, &iter, text, -1);
    gtk_text_buffer_insert(buffer, &iter, "\n", -1);
    
    GtkTextMark *mark = gtk_text_buffer_create_mark(buffer, NULL, &iter, FALSE);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(text_view), mark, 0.0, FALSE, 0.0, 0.0);
    gtk_text_buffer_delete_mark(buffer, mark);
}

// Callbacks
static void on_generate_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    TabContext *tab = (TabContext *)data;

    int categorie = (int)(tab - tabs) + 1;
    const char *question = get_local_question(categorie);

    append_log("> Question générée instantanément.");
    if (GTK_IS_LABEL(tab->label_question)) gtk_label_set_text(GTK_LABEL(tab->label_question), question);
    gtk_entry_set_text(GTK_ENTRY(tab->entry_reponse), "");
}

static void on_submit_clicked(GtkWidget *widget, gpointer data) {
    (void)widget;
    TabContext *tab = (TabContext *)data;
    
    const gchar *nom = gtk_entry_get_text(GTK_ENTRY(entry_nom));
    const gchar *reponse = gtk_entry_get_text(GTK_ENTRY(tab->entry_reponse));

    if (strlen(nom) == 0 || strlen(reponse) == 0) {
        append_log("[Système] Veuillez renseigner votre nom en haut, et votre réponse.");
        return;
    }

    char log_msg[512];
    snprintf(log_msg, sizeof(log_msg), "[%s] Réponse : %s", nom, reponse);
    append_log(log_msg);
    append_log("> Évaluation de la réponse instantanée...");

    gtk_entry_set_text(GTK_ENTRY(tab->entry_reponse), "");

    // Local simulated evaluation logic for responsiveness
    int score_simule;
    const char *eval_text;
    size_t len = strlen(reponse);

    if (len > 30) {
        score_simule = 9;
        eval_text = "Excellente analyse, les points essentiels sont bien développés. Parfait !";
    } else if (len > 10) {
        score_simule = 6;
        eval_text = "Bonne piste, mais la réponse manque un peu de complétude technique.";
    } else {
        score_simule = 3;
        eval_text = "Réponse bien trop courte, il faut développer ton raisonnement.";
    }

    append_log("[IA Aura] Évaluation :");
    append_log(eval_text);
            
    const char* cat_name = "Inconnu";
    if (tab->category) {
        cat_name = tab->category;
    }
    save_score(nom, score_simule, cat_name);
            
    char score_msg[128];
    snprintf(score_msg, sizeof(score_msg), "[Système] Score de %d/10 sauvegardé en BDD pour %s.", score_simule, nom);
    append_log(score_msg);

    // Mise à jour des compteurs et compte rendu tous les 30 questions
    global_questions_answered++;
    if (score_simule > 5) {
        global_correct_answers++;
    } else {
        global_incorrect_answers++;
    }

    if (global_questions_answered > 0 && global_questions_answered % 30 == 0) {
        char report_msg[512];
        snprintf(report_msg, sizeof(report_msg), 
            "\n================ COMPTE RENDU =================\n"
            "Questions répondues : %d\n"
            "Réponses correctes (score > 5) : %d\n"
            "Réponses incorrectes (score <= 5) : %d\n"
            "Taux de réussite : %.1f%%\n"
            "===============================================\n",
            global_questions_answered, global_correct_answers, global_incorrect_answers,
            (float)global_correct_answers / global_questions_answered * 100.0);
        append_log(report_msg);
    }
}

static void on_historique_clicked(GtkWidget *widget, gpointer data) {
    (void)widget; (void)data;
    append_log("[Système] L'historique s'affiche dans la console MSYS2.");
    show_scores();
}

int main(int argc, char *argv[]) {
    // Initialize GTK first
    if (!gtk_init_check(&argc, &argv)) {
        fprintf(stderr, "[AURA ERROR] GTK initialization failed\n");
        return 1;
    }
    
    printf("[AURA] GTK initialized successfully\n");
    
    // Initialize all AURA subsystems through launcher
    if (!aura_launcher_init_all_systems()) {
        fprintf(stderr, "[AURA ERROR] Failed to initialize AURA systems\n");
        return 1;
    }
    
    // Get accounts path from filesystem module
    char accounts_path[1024];
    if (!aura_fs_get_data_path("accounts.txt", accounts_path, sizeof(accounts_path))) {
        fprintf(stderr, "[AURA ERROR] Failed to get accounts path\n");
        aura_launcher_cleanup_all_systems();
        return 1;
    }
    
    // Initialize authentication system with proper path
    if (!auth_init(accounts_path)) {
        fprintf(stderr, "[AURA WARNING] Auth initialization encountered issues\n");
    }
    
    // Initialize database
    init_db();
    srand((unsigned int)time(NULL));
    apply_aura_css();
    
    printf("[AURA] All initialization complete - launching application\n");
    
    // Launch the login screen first
    gboolean authenticated = FALSE;
    while (!authenticated) {
        if (!aura_launch_login_screen(&argc, &argv)) {
            fprintf(stderr, "[AURA] Login screen closed by user\n");
            break;
        }
        
        // If login succeeded, launch dashboard
        gboolean dashboard_requests_login = aura_launch_dashboard(&argc, &argv);

        if (aura_dashboard_restart_requested()) {
            aura_dashboard_clear_restart_request();
            continue;
        }

        if (!dashboard_requests_login) {
            authenticated = TRUE;
        }
    }
    
    // Cleanup
    printf("[AURA] Application shutting down\n");
    aura_launcher_cleanup_all_systems();
    
    printf("[AURA] Goodbye!\n");
    return 0;
}

#if 0
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "AURA Simulator - AI-Powered Training Platform");
    gtk_window_set_default_size(GTK_WINDOW(window), 1200, 920);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    add_class(window, "aura-app");

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_set_spacing(GTK_BOX(vbox), 18);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 18);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    add_class(vbox, "aura-root");

    GtkWidget *hero = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(hero), 22);
    add_class(hero, "hero-card");
    gtk_box_pack_start(GTK_BOX(vbox), hero, FALSE, FALSE, 0);

    GtkWidget *hero_top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_box_pack_start(GTK_BOX(hero), hero_top, FALSE, FALSE, 0);

    GtkWidget *hero_stack = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_box_pack_start(GTK_BOX(hero_top), hero_stack, TRUE, TRUE, 0);

    GtkWidget *kicker = gtk_label_new("AURA • AI-POWERED TRAINING SIMULATOR");
    gtk_widget_set_halign(kicker, GTK_ALIGN_START);
    add_class(kicker, "aura-kicker");
    gtk_box_pack_start(GTK_BOX(hero_stack), kicker, FALSE, FALSE, 0);

    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_use_markup(GTK_LABEL(title), TRUE);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='36pt' weight='900'>AURA <span foreground='#00d9ff'>Simulator</span></span>");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(hero_stack), title, FALSE, FALSE, 0);

    GtkWidget *subtitle = gtk_label_new("Master interview skills with AI-driven questions, intelligent scoring, and comprehensive progress tracking.");
    gtk_widget_set_halign(subtitle, GTK_ALIGN_START);
    gtk_label_set_line_wrap(GTK_LABEL(subtitle), TRUE);
    add_class(subtitle, "aura-subtitle");
    gtk_box_pack_start(GTK_BOX(hero_stack), subtitle, FALSE, FALSE, 0);

    GtkWidget *hero_badges = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    add_class(hero_badges, "chip-row");
    gtk_box_pack_start(GTK_BOX(hero), hero_badges, FALSE, FALSE, 0);

    GtkWidget *badge_1 = gtk_label_new("� AI-Powered Generator");
    GtkWidget *badge_2 = gtk_label_new("📊 Smart Scoring & Feedback");
    GtkWidget *badge_3 = gtk_label_new("📈 Progress Tracking");
    add_class(badge_1, "hero-badge");
    add_class(badge_2, "hero-badge");
    add_class(badge_3, "hero-badge");
    gtk_box_pack_start(GTK_BOX(hero_badges), badge_1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hero_badges), badge_2, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hero_badges), badge_3, FALSE, FALSE, 0);

    GtkWidget *hero_actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
    gtk_box_set_spacing(GTK_BOX(hero_actions), 16);
    gtk_box_pack_start(GTK_BOX(hero), hero_actions, FALSE, FALSE, 0);

    GtkWidget *name_block = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_box_pack_start(GTK_BOX(hero_actions), name_block, TRUE, TRUE, 0);

    GtkWidget *label_nom = gtk_label_new("Trainee Profile");
    gtk_widget_set_halign(label_nom, GTK_ALIGN_START);
    add_class(label_nom, "field-label");
    gtk_box_pack_start(GTK_BOX(name_block), label_nom, FALSE, FALSE, 0);

    entry_nom = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_nom), "Nom complet");
    add_class(entry_nom, "name-entry");
    gtk_box_pack_start(GTK_BOX(name_block), entry_nom, FALSE, FALSE, 0);

    GtkWidget *btn_hist = gtk_button_new_with_label("📊 Performance Report");
    g_signal_connect(btn_hist, "clicked", G_CALLBACK(on_historique_clicked), NULL);
    add_class(btn_hist, "secondary-action");
    gtk_widget_set_halign(btn_hist, GTK_ALIGN_END);
    gtk_widget_set_valign(btn_hist, GTK_ALIGN_END);
    gtk_box_pack_end(GTK_BOX(hero_actions), btn_hist, FALSE, FALSE, 0);

    GtkWidget *panel_tabs = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(panel_tabs), 18);
    add_class(panel_tabs, "panel-card");
    gtk_box_pack_start(GTK_BOX(vbox), panel_tabs, TRUE, TRUE, 0);

    GtkWidget *panel_tabs_head = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_box_pack_start(GTK_BOX(panel_tabs), panel_tabs_head, FALSE, FALSE, 0);

    GtkWidget *panel_title = gtk_label_new("Training Simulator");
    gtk_widget_set_halign(panel_title, GTK_ALIGN_START);
    add_class(panel_title, "panel-heading");
    gtk_box_pack_start(GTK_BOX(panel_tabs_head), panel_title, FALSE, FALSE, 0);

    GtkWidget *panel_copy = gtk_label_new("Choose a domain, get an AI-generated question, provide your answer, and receive intelligent feedback with a score.");
    gtk_widget_set_halign(panel_copy, GTK_ALIGN_START);
    add_class(panel_copy, "panel-copy");
    gtk_box_pack_start(GTK_BOX(panel_tabs_head), panel_copy, FALSE, FALSE, 0);

    // LE NOTEBOOK (ONGLETS)
    GtkWidget *notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
    gtk_box_pack_start(GTK_BOX(panel_tabs), notebook, TRUE, TRUE, 0);

    const char* categories[] = {"Algorithmique", "Énigmes Mathématiques", "Culture Générale IT"};
    for (int i = 0; i < 3; i++) {
        tabs[i].category = categories[i];

        GtkWidget *page_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
        gtk_container_set_border_width(GTK_CONTAINER(page_vbox), 16);
        add_class(page_vbox, "tab-page");

        // Step 1: Generate Question Button
        GtkWidget *step1_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        GtkWidget *step1_label = gtk_label_new("STEP 1: AI-Generated Question");
        gtk_widget_set_halign(step1_label, GTK_ALIGN_START);
        add_class(step1_label, "field-label");
        gtk_box_pack_start(GTK_BOX(step1_box), step1_label, FALSE, FALSE, 0);
        
        tabs[i].btn_gen = gtk_button_new_with_label("Generate AI Question");
        gtk_box_pack_start(GTK_BOX(step1_box), tabs[i].btn_gen, FALSE, FALSE, 0);
        add_class(tabs[i].btn_gen, "primary-action");
        gtk_box_pack_start(GTK_BOX(page_vbox), step1_box, FALSE, FALSE, 0);

        // Question Display
        tabs[i].label_question = gtk_label_new("Press 'Generate AI Question' to get started...");
        gtk_label_set_line_wrap(GTK_LABEL(tabs[i].label_question), TRUE);
        gtk_widget_set_margin_top(tabs[i].label_question, 10);
        gtk_widget_set_margin_bottom(tabs[i].label_question, 10);
        gtk_widget_set_halign(tabs[i].label_question, GTK_ALIGN_START);
        gtk_box_pack_start(GTK_BOX(page_vbox), tabs[i].label_question, FALSE, FALSE, 0);
        add_class(tabs[i].label_question, "question-card");

        // Step 2: Answer Section
        GtkWidget *step2_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        GtkWidget *step2_label = gtk_label_new("STEP 2: Your Answer");
        gtk_widget_set_halign(step2_label, GTK_ALIGN_START);
        add_class(step2_label, "field-label");
        gtk_box_pack_start(GTK_BOX(step2_box), step2_label, FALSE, FALSE, 0);
        
        tabs[i].entry_reponse = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(tabs[i].entry_reponse), "Saisissez votre reponse ici...");
        gtk_box_pack_start(GTK_BOX(step2_box), tabs[i].entry_reponse, FALSE, FALSE, 0);
        add_class(tabs[i].entry_reponse, "response-entry");
        gtk_box_pack_start(GTK_BOX(page_vbox), step2_box, FALSE, FALSE, 0);

        // Step 3: Submit Section
        GtkWidget *step3_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        GtkWidget *step3_label = gtk_label_new("STEP 3: Get AI Score & Feedback");
        gtk_widget_set_halign(step3_label, GTK_ALIGN_START);
        add_class(step3_label, "field-label");
        gtk_box_pack_start(GTK_BOX(step3_box), step3_label, FALSE, FALSE, 0);
        
        GtkWidget *hbox_acc = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        tabs[i].btn_submit = gtk_button_new_with_label("Submit for AI Evaluation");
        tabs[i].spinner = gtk_spinner_new();
        gtk_box_pack_start(GTK_BOX(hbox_acc), tabs[i].btn_submit, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(hbox_acc), tabs[i].spinner, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(step3_box), hbox_acc, FALSE, FALSE, 0);
        add_class(hbox_acc, "action-row");
        add_class(tabs[i].btn_submit, "primary-action");
        gtk_box_pack_start(GTK_BOX(page_vbox), step3_box, FALSE, FALSE, 0);

        g_signal_connect(tabs[i].btn_gen, "clicked", G_CALLBACK(on_generate_clicked), &tabs[i]);
        g_signal_connect(tabs[i].btn_submit, "clicked", G_CALLBACK(on_submit_clicked), &tabs[i]);

        GtkWidget *tab_label = gtk_label_new(categories[i]);
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_vbox, tab_label);
    }

    GtkWidget *log_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(log_card), 18);
    add_class(log_card, "log-card");
    gtk_box_pack_start(GTK_BOX(vbox), log_card, TRUE, TRUE, 0);

    GtkWidget *label_log = gtk_label_new("Feedback & Progress");
    gtk_widget_set_halign(label_log, GTK_ALIGN_START);
    add_class(label_log, "panel-heading");
    gtk_box_pack_start(GTK_BOX(log_card), label_log, FALSE, FALSE, 0);

    GtkWidget *log_copy = gtk_label_new("Watch your AI feedback in real-time, track your scores, and monitor your learning progress across all sessions.");
    gtk_widget_set_halign(log_copy, GTK_ALIGN_START);
    add_class(log_copy, "panel-copy");
    gtk_box_pack_start(GTK_BOX(log_card), log_copy, FALSE, FALSE, 0);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scroll, TRUE);
    add_class(scroll, "log-scroller");
    text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    add_class(text_view, "log-view");
    gtk_container_add(GTK_CONTAINER(scroll), text_view);
    gtk_box_pack_start(GTK_BOX(log_card), scroll, TRUE, TRUE, 0);

    append_log("═════════════════════════════════════════════════════");
    append_log("  🤖 AURA SIMULATOR - AI-Powered Training Platform");
    append_log("═════════════════════════════════════════════════════");
    append_log("");
    append_log("✓ Choose a training domain (Algorithms, Math, or IT)");
    append_log("✓ Generate an AI-powered question tailored to you");
    append_log("✓ Provide your best answer");
    append_log("✓ Get intelligent AI-driven feedback and a score");
    append_log("✓ Track your progress and improve");
    append_log("");
    append_log("═════════════════════════════════════════════════════");

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
#endif