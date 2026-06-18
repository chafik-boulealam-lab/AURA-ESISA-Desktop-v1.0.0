#include "report.h"
#include "evaluation.h"
#include "filesystem.h"
#include <cairo/cairo-pdf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <glib.h>

static const char *verdict_label(int v) {
    if (v == VERDICT_CORRECT) return "VRAI";
    if (v == VERDICT_PARTIAL) return "PARTIEL";
    if (v == VERDICT_INCORRECT) return "FAUX";
    return "—";
}

static void draw_wrapped(cairo_t *cr, const char *text, double x, double *y, double line_h) {
    if (!text || !text[0]) {
        cairo_move_to(cr, x, *y);
        cairo_show_text(cr, "(vide)");
        *y += line_h;
        return;
    }
    char buf[4096];
    strncpy(buf, text, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    char *word = buf;
    char line[1024];
    line[0] = '\0';
    size_t line_len = 0;

    while (*word) {
        while (*word == ' ') word++;
        if (!*word) break;
        char *sp = strchr(word, ' ');
        size_t wlen = sp ? (size_t)(sp - word) : strlen(word);
        char token[256];
        if (wlen >= sizeof(token)) wlen = sizeof(token) - 1;
        memcpy(token, word, wlen);
        token[wlen] = '\0';

        if (line_len + wlen + (line_len ? 1 : 0) > 88) {
            cairo_move_to(cr, x, *y);
            cairo_show_text(cr, line);
            *y += line_h;
            line[0] = '\0';
            line_len = 0;
        }
        if (line_len > 0) { strcat(line, " "); line_len++; }
        strcat(line, token);
        line_len += wlen;
        word = sp ? sp + 1 : word + wlen;
    }
    if (line[0]) {
        cairo_move_to(cr, x, *y);
        cairo_show_text(cr, line);
        *y += line_h;
    }
}

bool report_generate_session_pdf(Interview *iv, int avg_score, char *out_path, size_t path_len) {
    if (!iv || !out_path || path_len == 0) return false;

    char reports_dir[1024];
    if (!aura_fs_get_data_path("reports", reports_dir, sizeof(reports_dir)))
        return false;
    aura_fs_mkdir_recursive(reports_dir);

    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char filename[128];
    snprintf(filename, sizeof(filename), "session_%04d%02d%02d_%02d%02d%02d.pdf",
        tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
        tm->tm_hour, tm->tm_min, tm->tm_sec);

    char fullpath[1200];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", reports_dir, filename);

    cairo_surface_t *surface = cairo_pdf_surface_create(fullpath, 595, 842);
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        if (surface) cairo_surface_destroy(surface);
        return false;
    }

    cairo_t *cr = cairo_create(surface);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    double y = 50;

    cairo_set_font_size(cr, 20);
    cairo_move_to(cr, 50, y);
    cairo_show_text(cr, "ESISA - Rapport d'entretien AURA");
    y += 32;

    cairo_set_font_size(cr, 12);
    char meta[512];
    snprintf(meta, sizeof(meta), "Etudiant: %s",
        interview_get_user(iv) ? interview_get_user(iv) : "-");
    cairo_move_to(cr, 50, y); cairo_show_text(cr, meta); y += 18;
    snprintf(meta, sizeof(meta), "Matiere: %s  |  Niveau: %s",
        interview_get_domain(iv) ? interview_get_domain(iv) : "-",
        interview_get_level(iv) ? interview_get_level(iv) : "-");
    cairo_move_to(cr, 50, y); cairo_show_text(cr, meta); y += 18;
    snprintf(meta, sizeof(meta), "Score moyen: %d/10  |  Date: %04d-%02d-%02d %02d:%02d",
        avg_score, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min);
    cairo_move_to(cr, 50, y); cairo_show_text(cr, meta); y += 28;

    int n = interview_get_questions_count(iv);
    for (int i = 0; i < n; i++) {
        if (y > 750) {
            cairo_show_page(cr);
            y = 50;
        }
        cairo_set_font_size(cr, 13);
        char qtitle[128];
        snprintf(qtitle, sizeof(qtitle), "Question %d - %s (%d/10)",
            i + 1, verdict_label(interview_get_verdict(iv, i)), interview_get_score(iv, i));
        cairo_move_to(cr, 50, y); cairo_show_text(cr, qtitle); y += 20;

        cairo_set_font_size(cr, 11);
        char qtext[1024];
        interview_get_question_text(iv, i, qtext, sizeof(qtext));
        char block[2048];
        snprintf(block, sizeof(block), "Enonce: %s", qtext);
        draw_wrapped(cr, block, 50, &y, 16);

        const char *ans = interview_get_answer(iv, i);
        snprintf(block, sizeof(block), "Reponse: %s", ans ? ans : "(aucune)");
        draw_wrapped(cr, block, 50, &y, 16);

        const char *fb = interview_get_feedback(iv, i);
        snprintf(block, sizeof(block), "Correction: %s", fb ? fb : "-");
        draw_wrapped(cr, block, 50, &y, 16);
        y += 10;
    }

    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    strncpy(out_path, fullpath, path_len - 1);
    out_path[path_len - 1] = '\0';
    return true;
}

static int cmp_str_desc(const void *a, const void *b) {
    return strcmp((const char *)b, (const char *)a);
}

int report_list_pdfs(char paths[][1024], int max_paths) {
    if (!paths || max_paths <= 0) return 0;
    char reports_dir[1024];
    if (!aura_fs_get_data_path("reports", reports_dir, sizeof(reports_dir)))
        return 0;
    if (!aura_fs_dir_exists(reports_dir)) return 0;

    GDir *dir = g_dir_open(reports_dir, 0, NULL);
    if (!dir) return 0;

    char tmp[64][1024];
    int count = 0;
    const gchar *name;
    while ((name = g_dir_read_name(dir)) != NULL && count < 64) {
        if (!g_str_has_suffix(name, ".pdf")) continue;
        snprintf(tmp[count], sizeof(tmp[count]), "%s/%s", reports_dir, name);
        count++;
    }
    g_dir_close(dir);

    qsort(tmp, (size_t)count, sizeof(tmp[0]), cmp_str_desc);
    int n = count < max_paths ? count : max_paths;
    for (int i = 0; i < n; i++)
        strncpy(paths[i], tmp[i], 1023);
    return n;
}

bool report_open_file(const char *path) {
    if (!path || !path[0]) return false;
#ifdef _WIN32
    char cmd[1400];
    snprintf(cmd, sizeof(cmd), "cmd /c start \"\" \"%s\"", path);
#else
    char cmd[1400];
    snprintf(cmd, sizeof(cmd), "xdg-open \"%s\"", path);
#endif
    GError *err = NULL;
    if (!g_spawn_command_line_async(cmd, &err)) {
        if (err) g_error_free(err);
        return false;
    }
    return true;
}