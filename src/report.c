#include "report.h"
#include "evaluation.h"
#include "filesystem.h"
#include <cairo/cairo-pdf.h>
#include <pango/pangocairo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <glib.h>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

static const char *verdict_label(int v) {
    if (v == VERDICT_CORRECT) return "VRAI";
    if (v == VERDICT_PARTIAL) return "PARTIEL";
    if (v == VERDICT_INCORRECT) return "FAUX";
    return "-";
}

static void pdf_draw_text(cairo_t *cr, PangoLayout *layout, const char *text,
    double x, double *y, int max_width_px) {
    if (!text) text = "";
    pango_layout_set_width(layout, max_width_px * PANGO_SCALE);
    pango_layout_set_text(layout, text, -1);
    int h = 0;
    pango_layout_get_pixel_size(layout, NULL, &h);
    cairo_move_to(cr, x, *y);
    pango_cairo_show_layout(cr, layout);
    *y += (double)h + 6.0;
}

static bool write_txt_report(Interview *iv, int avg_score, const char *basepath) {
    char txtpath[1200];
    snprintf(txtpath, sizeof(txtpath), "%s.txt", basepath);
    FILE *f = fopen(txtpath, "w");
    if (!f) return false;

    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    fprintf(f, "ESISA - Rapport d'entretien AURA\n");
    fprintf(f, "Etudiant: %s\n", interview_get_user(iv) ? interview_get_user(iv) : "-");
    fprintf(f, "Matiere: %s | Niveau: %s\n",
        interview_get_domain(iv) ? interview_get_domain(iv) : "-",
        interview_get_level(iv) ? interview_get_level(iv) : "-");
    fprintf(f, "Score moyen: %d/10\n", avg_score);
    fprintf(f, "Date: %04d-%02d-%02d %02d:%02d\n\n",
        tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min);

    int n = interview_get_questions_count(iv);
    for (int i = 0; i < n; i++) {
        char qtext[1024];
        interview_get_question_text(iv, i, qtext, sizeof(qtext));
        fprintf(f, "--- Question %d [%s] %d/10 ---\n", i + 1,
            verdict_label(interview_get_verdict(iv, i)), interview_get_score(iv, i));
        fprintf(f, "Enonce: %s\n", qtext);
        fprintf(f, "Reponse: %s\n", interview_get_answer(iv, i) ? interview_get_answer(iv, i) : "(aucune)");
        fprintf(f, "Correction: %s\n\n", interview_get_feedback(iv, i) ? interview_get_feedback(iv, i) : "-");
    }
    fclose(f);
    return true;
}

bool report_generate_session_pdf(Interview *iv, int avg_score, char *out_path, size_t path_len) {
    if (!iv || !out_path || path_len == 0) return false;

    char reports_dir[1024];
    if (!aura_fs_get_data_path("reports", reports_dir, sizeof(reports_dir)))
        return false;
    aura_fs_mkdir_recursive(reports_dir);

    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char basename[128];
    snprintf(basename, sizeof(basename), "session_%04d%02d%02d_%02d%02d%02d",
        tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
        tm->tm_hour, tm->tm_min, tm->tm_sec);

    char basepath[1200];
    char fullpath[1200];
    snprintf(basepath, sizeof(basepath), "%s%s%s", reports_dir, PATH_SEP, basename);
    snprintf(fullpath, sizeof(fullpath), "%s.pdf", basepath);

    cairo_surface_t *surface = cairo_pdf_surface_create(fullpath, 595, 842);
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        fprintf(stderr, "[PDF] surface error: %s\n", cairo_status_to_string(cairo_surface_status(surface)));
        if (surface) cairo_surface_destroy(surface);
        if (write_txt_report(iv, avg_score, basepath)) {
            snprintf(fullpath, sizeof(fullpath), "%s.txt", basepath);
            strncpy(out_path, fullpath, path_len - 1);
            out_path[path_len - 1] = '\0';
            return true;
        }
        return false;
    }

    cairo_t *cr = cairo_create(surface);
    PangoLayout *layout = pango_cairo_create_layout(cr);
    PangoFontDescription *font = pango_font_description_from_string("Segoe UI 11");
    if (font) {
        pango_layout_set_font_description(layout, font);
        pango_font_description_free(font);
    }
    pango_layout_set_wrap(layout, PANGO_WRAP_WORD);
    double y = 40;

    pdf_draw_text(cr, layout, "ESISA - Rapport d'entretien AURA", 40, &y, 515);

    char meta[512];
    snprintf(meta, sizeof(meta), "Etudiant: %s",
        interview_get_user(iv) ? interview_get_user(iv) : "-");
    pdf_draw_text(cr, layout, meta, 40, &y, 515);
    snprintf(meta, sizeof(meta), "Matiere: %s  |  Niveau: %s",
        interview_get_domain(iv) ? interview_get_domain(iv) : "-",
        interview_get_level(iv) ? interview_get_level(iv) : "-");
    pdf_draw_text(cr, layout, meta, 40, &y, 515);
    snprintf(meta, sizeof(meta), "Score moyen: %d/10  |  Date: %04d-%02d-%02d %02d:%02d",
        avg_score, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min);
    pdf_draw_text(cr, layout, meta, 40, &y, 515);
    y += 10;

    int n = interview_get_questions_count(iv);
    for (int i = 0; i < n; i++) {
        if (y > 760) {
            cairo_show_page(cr);
            y = 40;
        }
        char qtitle[256];
        snprintf(qtitle, sizeof(qtitle), "Question %d - %s (%d/10)",
            i + 1, verdict_label(interview_get_verdict(iv, i)), interview_get_score(iv, i));
        pdf_draw_text(cr, layout, qtitle, 40, &y, 515);

        char qtext[1024];
        interview_get_question_text(iv, i, qtext, sizeof(qtext));
        char block[4096];
        snprintf(block, sizeof(block), "Enonce: %s", qtext);
        pdf_draw_text(cr, layout, block, 40, &y, 515);

        const char *ans = interview_get_answer(iv, i);
        snprintf(block, sizeof(block), "Reponse: %s", ans ? ans : "(aucune)");
        pdf_draw_text(cr, layout, block, 40, &y, 515);

        const char *fb = interview_get_feedback(iv, i);
        snprintf(block, sizeof(block), "Correction: %s", fb ? fb : "-");
        pdf_draw_text(cr, layout, block, 40, &y, 515);
        y += 8;
    }

    g_object_unref(layout);
    cairo_destroy(cr);
    cairo_surface_finish(surface);
    cairo_status_t st = cairo_surface_status(surface);
    cairo_surface_destroy(surface);

    if (st != CAIRO_STATUS_SUCCESS) {
        fprintf(stderr, "[PDF] write error: %s\n", cairo_status_to_string(st));
        if (write_txt_report(iv, avg_score, basepath)) {
            snprintf(fullpath, sizeof(fullpath), "%s.txt", basepath);
            strncpy(out_path, fullpath, path_len - 1);
            out_path[path_len - 1] = '\0';
            return true;
        }
        return false;
    }

    if (!aura_fs_file_exists(fullpath)) {
        fprintf(stderr, "[PDF] file not found after write: %s\n", fullpath);
        if (write_txt_report(iv, avg_score, basepath)) {
            snprintf(fullpath, sizeof(fullpath), "%s.txt", basepath);
            strncpy(out_path, fullpath, path_len - 1);
            out_path[path_len - 1] = '\0';
            return true;
        }
        return false;
    }

    printf("[PDF] Rapport genere: %s\n", fullpath);
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
        if (!g_str_has_suffix(name, ".pdf") && !g_str_has_suffix(name, ".txt")) continue;
        snprintf(tmp[count], sizeof(tmp[count]), "%s%s%s", reports_dir, PATH_SEP, name);
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
    HINSTANCE rc = ShellExecuteA(NULL, "open", path, NULL, NULL, SW_SHOWNORMAL);
    if ((INT_PTR)rc <= 32) {
        fprintf(stderr, "[PDF] open error: ShellExecute failed (%d) for %s\n",
            (int)(INT_PTR)rc, path);
        return false;
    }
    return true;
#else
    char cmd[1400];
    snprintf(cmd, sizeof(cmd), "xdg-open \"%s\"", path);
    GError *err = NULL;
    if (!g_spawn_command_line_async(cmd, &err)) {
        if (err) {
            fprintf(stderr, "[PDF] open error: %s\n", err->message);
            g_error_free(err);
        }
        return false;
    }
    return true;
#endif
}