#include "questions.h"
#include "question_bank.h"
#include "filesystem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

static Question *g_questions = NULL;
static size_t g_qcount = 0;

static char *strdup_safe(const char *s) {
    if (!s) return NULL;
    char *d = malloc(strlen(s) + 1);
    if (d) strcpy(d, s);
    return d;
}

static void trim_inplace(char *s) {
    if (!s) return;
    char *start = s;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != s) memmove(s, start, strlen(start) + 1);
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[--len] = '\0';
    }
}

static void add_question(const char *id, const char *domain, const char *level,
                         const char *text, const char *answer) {
    size_t nc = g_qcount + 1;
    Question *tmp = realloc(g_questions, nc * sizeof(Question));
    if (!tmp) return;
    g_questions = tmp;
    Question *q = &g_questions[g_qcount];
    q->id = strdup_safe(id);
    q->domain = strdup_safe(domain ? domain : "");
    q->level = strdup_safe(level ? level : "");
    q->text = strdup_safe(text ? text : "");
    q->choices = NULL;
    q->choices_count = 0;
    q->answer = strdup_safe(answer ? answer : "");
    g_qcount++;
}

static void load_builtin_defaults(void) {
    static const char *rows[][5] = {
        {"1", "Architecture des ordinateurs", "Junior", "Quelle est la difference entre la RAM et le disque dur?", "RAM volatile et rapide, disque persistant."},
        {"2", "Architecture des ordinateurs", "Intermediate", "Expliquez le cycle Fetch-Decode-Execute.", "Le CPU charge, decode puis execute l'instruction."},
        {"3", "Architecture des ordinateurs", "Advanced", "Comparez Harvard et Von Neumann.", "Harvard separe bus instructions/donnees."},
        {"7", "Algorithmique", "Junior", "Quelle est la complexite de la recherche lineaire?", "O(n)"},
        {"9", "Algorithmique", "Intermediate", "Expliquez la difference entre DFS et BFS.", "DFS profondeur, BFS largeur."},
        {"11", "Algorithmique", "Advanced", "Qu'est-ce que la programmation dynamique?", "Sous-problemes avec memoisation."},
        {"14", "Programmation C", "Junior", "Qu'est-ce qu'un pointeur en C?", "Variable contenant une adresse memoire."},
        {"15", "Programmation C", "Intermediate", "Expliquez malloc et free.", "Allocation et liberation dynamique."},
        {"17", "Programmation C", "Advanced", "Qu'est-ce qu'une fuite memoire?", "Memoire allouee jamais liberee."},
    };
    for (size_t i = 0; i < sizeof(rows) / sizeof(rows[0]); i++) {
        add_question(rows[i][0], rows[i][1], rows[i][2], rows[i][3], rows[i][4]);
    }
}

static bool parse_csv_line(char *line) {
    char *nl = strchr(line, '\n');
    if (nl) *nl = '\0';
    char *cr = strchr(line, '\r');
    if (cr) *cr = '\0';
    trim_inplace(line);
    if (line[0] == '\0' || line[0] == '#') return false;

    char *c1 = strchr(line, ',');
    if (!c1) return false;
    *c1 = '\0';
    char *c2 = strchr(c1 + 1, ',');
    if (!c2) return false;
    *c2 = '\0';
    char *c3 = strchr(c2 + 1, ',');
    if (!c3) return false;
    *c3 = '\0';
    char *c4 = strchr(c3 + 1, ',');
    if (!c4) return false;
    *c4 = '\0';

    char *id = line;
    char *domain = c1 + 1;
    char *level = c2 + 1;
    char *text = c3 + 1;
    char *answer = c4 + 1;
    trim_inplace(id);
    trim_inplace(domain);
    trim_inplace(level);
    trim_inplace(text);
    trim_inplace(answer);
    if (!text[0]) return false;
    add_question(id, domain, level, text, answer);
    return true;
}

bool questions_load(const char *path) {
    const char *p = path ? path : "data/questions.csv";
    FILE *f = fopen(p, "r");
    if (!f) return false;

    questions_free_all();
    char line[4096];
    bool header_skipped = false;
    while (fgets(line, sizeof(line), f)) {
        if (!header_skipped) {
            if (strncmp(line, "id,", 3) == 0) {
                header_skipped = true;
                continue;
            }
            header_skipped = true;
        }
        parse_csv_line(line);
    }
    fclose(f);
    if (g_qcount > 0) srand((unsigned)time(NULL));
    return g_qcount > 0;
}

bool questions_ensure_loaded(void) {
    if (g_qcount > 0) return true;

    char csv_path[1024];
    const char *candidates[] = {
        NULL,
        "data/questions.csv",
        "data\\questions.csv",
        NULL
    };

    if (aura_fs_get_data_path("questions.csv", csv_path, sizeof(csv_path))) {
        if (questions_load(csv_path)) return true;
    }
    for (int i = 0; candidates[i]; i++) {
        if (questions_load(candidates[i])) return true;
    }

    load_builtin_defaults();
    if (g_qcount > 0) srand((unsigned)time(NULL));
    return g_qcount > 0;
}

int questions_count(void) {
    return (int)g_qcount;
}

Question *questions_get_random(const char *domain, const char *level) {
    if (g_qcount == 0 && !questions_ensure_loaded()) return NULL;

    size_t matches_cap = 0, matches_len = 0;
    size_t *matches = NULL;
    for (size_t i = 0; i < g_qcount; ++i) {
        Question *q = &g_questions[i];
        if (domain && domain[0] && q->domain && strcmp(domain, q->domain) != 0) continue;
        if (level && level[0] && q->level && strcmp(level, q->level) != 0) continue;
        if (matches_len + 1 > matches_cap) {
            size_t nc = matches_cap ? matches_cap * 2 : 16;
            size_t *t = realloc(matches, nc * sizeof(size_t));
            if (!t) { free(matches); return NULL; }
            matches = t;
            matches_cap = nc;
        }
        matches[matches_len++] = i;
    }

    if (matches_len == 0) {
        for (size_t i = 0; i < g_qcount; ++i) {
            Question *q = &g_questions[i];
            if (domain && domain[0] && q->domain && strcmp(domain, q->domain) != 0) continue;
            if (matches_len + 1 > matches_cap) {
                size_t nc = matches_cap ? matches_cap * 2 : 16;
                size_t *t = realloc(matches, nc * sizeof(size_t));
                if (!t) { free(matches); return NULL; }
                matches = t;
                matches_cap = nc;
            }
            matches[matches_len++] = i;
        }
    }

    if (matches_len == 0) { free(matches); return NULL; }
    size_t pick = matches[rand() % matches_len];
    Question *res = &g_questions[pick];
    free(matches);
    return res;
}

bool questions_pick_for_interview(const char *domain, const char *level,
    const int *exclude_ids, int exclude_count,
    int *out_id, char *out_text, size_t text_len,
    char *out_ref, size_t ref_len, const char *username) {
    qb_init();
    if (qb_pick_unique(domain, level, exclude_ids, exclude_count,
            out_id, out_text, text_len, out_ref, ref_len, username)) {
        return true;
    }
    Question *q = questions_get_random(domain, level);
    if (!q || !q->text || !q->text[0]) return false;
    if (out_id) *out_id = q->id ? atoi(q->id) : 0;
    strncpy(out_text, q->text, text_len - 1);
    out_text[text_len - 1] = '\0';
    if (out_ref && ref_len > 0) {
        strncpy(out_ref, q->answer ? q->answer : "", ref_len - 1);
        out_ref[ref_len - 1] = '\0';
    }
    return true;
}

void questions_free_all(void) {
    if (!g_questions) return;
    for (size_t i = 0; i < g_qcount; ++i) {
        Question *q = &g_questions[i];
        free(q->id);
        free(q->domain);
        free(q->level);
        free(q->text);
        free(q->answer);
        if (q->choices) {
            for (int j = 0; j < q->choices_count; j++) free(q->choices[j]);
            free(q->choices);
        }
    }
    free(g_questions);
    g_questions = NULL;
    g_qcount = 0;
}