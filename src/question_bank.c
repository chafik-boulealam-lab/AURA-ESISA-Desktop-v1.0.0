#include "question_bank.h"
#include "question_seed.h"
#include "filesystem.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char *DOMAINS[] = {
    "Architecture des ordinateurs",
    "Algorithmique",
    "Programmation C"
};
static const char *LEVELS[] = { "Junior", "Intermediate", "Advanced", "Expert" };

static bool ensure_tables(sqlite3 *db) {
    const char *sql_bank =
        "CREATE TABLE IF NOT EXISTS question_bank ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "domain TEXT NOT NULL,"
        "level TEXT NOT NULL,"
        "text TEXT NOT NULL UNIQUE,"
        "reference_answer TEXT,"
        "topic TEXT,"
        "source TEXT DEFAULT 'seed'"
        ");";
    const char *sql_hist =
        "CREATE TABLE IF NOT EXISTS user_question_history ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "username TEXT NOT NULL,"
        "question_id INTEGER NOT NULL,"
        "answered_ts INTEGER DEFAULT (strftime('%s','now')),"
        "UNIQUE(username, question_id)"
        ");";
    const char *sql_answers =
        "CREATE TABLE IF NOT EXISTS answer_details ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_name TEXT NOT NULL,"
        "question_id INTEGER,"
        "domain TEXT,"
        "level TEXT,"
        "verdict INTEGER,"
        "score INTEGER,"
        "feedback TEXT,"
        "answered_ts INTEGER DEFAULT (strftime('%s','now'))"
        ");";
    char *err = NULL;
    if (sqlite3_exec(db, sql_bank, NULL, NULL, &err) != SQLITE_OK) {
        fprintf(stderr, "[QB] bank table: %s\n", err ? err : "error");
        sqlite3_free(err);
        return false;
    }
    if (sqlite3_exec(db, sql_hist, NULL, NULL, &err) != SQLITE_OK) {
        fprintf(stderr, "[QB] history table: %s\n", err ? err : "error");
        sqlite3_free(err);
        return false;
    }
    if (sqlite3_exec(db, sql_answers, NULL, NULL, &err) != SQLITE_OK) {
        fprintf(stderr, "[QB] answers table: %s\n", err ? err : "error");
        sqlite3_free(err);
        return false;
    }
    sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_qb_domain_level ON question_bank(domain, level);", NULL, NULL, NULL);
    sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_uqh_user ON user_question_history(username);", NULL, NULL, NULL);
    return true;
}

static bool qb_open_db(sqlite3 **out_db) {
    char path[1024];
    if (!aura_fs_get_data_path("local.db", path, sizeof(path))) return false;
    if (sqlite3_open(path, out_db) != SQLITE_OK) return false;
    return ensure_tables(*out_db);
}

static int count_domain_level(sqlite3 *db, const char *domain, const char *level) {
    const char *sql = "SELECT COUNT(1) FROM question_bank WHERE domain=? AND level=?;";
    sqlite3_stmt *stmt;
    int n = 0;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, domain, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, level, -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) n = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    return n;
}

static void seed_all_if_needed(sqlite3 *db) {
    char csv_path[1024];
    if (aura_fs_get_data_path("questions.csv", csv_path, sizeof(csv_path))) {
        qb_import_csv(db, csv_path);
    }
    qb_import_csv(db, "data/questions.csv");
    qb_import_csv(db, "data\\questions.csv");

    for (int d = 0; d < 3; d++) {
        for (int l = 0; l < 4; l++) {
            int cur = count_domain_level(db, DOMAINS[d], LEVELS[l]);
            if (cur < QB_MIN_PER_LEVEL) {
                int need = QB_MIN_PER_LEVEL - cur;
                printf("[QB] Seeding %s / %s: +%d (had %d)\n", DOMAINS[d], LEVELS[l], need, cur);
                qb_seed_domain_level(db, DOMAINS[d], LEVELS[l], need + 50);
            }
        }
    }
}

bool qb_init(void) {
    sqlite3 *db = NULL;
    if (!qb_open_db(&db)) {
        fprintf(stderr, "[QB] Cannot open database\n");
        return false;
    }
    seed_all_if_needed(db);
    printf("[QB] Question bank ready\n");
    sqlite3_close(db);
    return true;
}

int qb_count(const char *domain, const char *level) {
    sqlite3 *db = NULL;
    if (!qb_open_db(&db)) return 0;
    int n = count_domain_level(db, domain, level);
    sqlite3_close(db);
    return n;
}

static bool bind_exclude(sqlite3_stmt *stmt, int start_idx, const int *exclude_ids, int exclude_count) {
    for (int i = 0; i < exclude_count; i++) {
        sqlite3_bind_int(stmt, start_idx + i, exclude_ids[i]);
    }
    return true;
}

static bool pick_query(sqlite3 *db, const char *domain, const char *level,
    const int *exclude_ids, int exclude_count, const char *username,
    bool use_history, int *out_id, char *out_text, size_t text_len,
    char *out_ref, size_t ref_len) {

    char sql[2048];
    int off = snprintf(sql, sizeof(sql),
        "SELECT id, text, reference_answer FROM question_bank "
        "WHERE domain=? AND level=? ");
    if (exclude_count > 0) {
        off += snprintf(sql + off, sizeof(sql) - (size_t)off, "AND id NOT IN (");
        for (int i = 0; i < exclude_count; i++) {
            if (i > 0) off += snprintf(sql + off, sizeof(sql) - (size_t)off, ",");
            off += snprintf(sql + off, sizeof(sql) - (size_t)off, "?");
        }
        off += snprintf(sql + off, sizeof(sql) - (size_t)off, ") ");
    }
    if (use_history && username && username[0]) {
        off += snprintf(sql + off, sizeof(sql) - (size_t)off,
            "AND id NOT IN (SELECT question_id FROM user_question_history WHERE username=?) ");
    }
    snprintf(sql + off, sizeof(sql) - (size_t)off, "ORDER BY RANDOM() LIMIT 1;");

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return false;

    int bi = 1;
    sqlite3_bind_text(stmt, bi++, domain, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, bi++, level, -1, SQLITE_TRANSIENT);
    bind_exclude(stmt, bi, exclude_ids, exclude_count);
    bi += exclude_count;
    if (use_history && username && username[0])
        sqlite3_bind_text(stmt, bi++, username, -1, SQLITE_TRANSIENT);

    bool ok = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        *out_id = sqlite3_column_int(stmt, 0);
        const char *txt = (const char *)sqlite3_column_text(stmt, 1);
        const char *ref = (const char *)sqlite3_column_text(stmt, 2);
        if (txt) {
            strncpy(out_text, txt, text_len - 1);
            out_text[text_len - 1] = '\0';
        } else {
            out_text[0] = '\0';
        }
        if (out_ref && ref_len > 0) {
            if (ref) {
                strncpy(out_ref, ref, ref_len - 1);
                out_ref[ref_len - 1] = '\0';
            } else {
                out_ref[0] = '\0';
            }
        }
        ok = out_text[0] != '\0';
    }
    sqlite3_finalize(stmt);
    return ok;
}

bool qb_pick_unique(const char *domain, const char *level,
    const int *exclude_ids, int exclude_count,
    int *out_id, char *out_text, size_t text_len,
    char *out_ref, size_t ref_len, const char *username) {

    if (!domain || !level || !out_id || !out_text || text_len == 0) return false;

    sqlite3 *db = NULL;
    if (!qb_open_db(&db)) return false;

    bool ok = pick_query(db, domain, level, exclude_ids, exclude_count, username,
        true, out_id, out_text, text_len, out_ref, ref_len);
    if (!ok)
        ok = pick_query(db, domain, level, exclude_ids, exclude_count, username,
            false, out_id, out_text, text_len, out_ref, ref_len);
    if (!ok)
        ok = pick_query(db, domain, level, NULL, 0, username,
            false, out_id, out_text, text_len, out_ref, ref_len);

    sqlite3_close(db);
    return ok;
}

void qb_mark_answered(const char *username, int question_id) {
    if (!username || !username[0] || question_id <= 0) return;
    sqlite3 *db = NULL;
    if (!qb_open_db(&db)) return;
    const char *sql =
        "INSERT OR IGNORE INTO user_question_history(username, question_id) VALUES(?, ?);";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, question_id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
}