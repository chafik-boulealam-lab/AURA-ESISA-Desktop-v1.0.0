#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string.h>
#include <time.h>
#include <glib.h>
#include "db.h"
#include "question_bank.h"
#include "filesystem.h"

// =====================================================================
// EXPLICATION SOUTENANCE : init_db()
// =====================================================================
// SQLite crée ou ouvre un système de fichiers local via sqlite3_open().
// 
// Mécanisme : 
// - On utilise aura_fs_get_data_path() pour obtenir le chemin correct
//   depuis le module filesystem, qui gère les répertoires de l'application.
// - `sqlite3 *db` : Pointeur qui contiendra l'instance connectée de SQLite.
// On passe son adresse (&db) en paramètre pour que la librairie SQLite
// y affecte le pointeur réel.
// 
// Création via sql brut : `sqlite3_exec` exécute du texte SQL brut car
// cette commande "CREATE TABLE" n'implique pas de saisies utilisateurs
// dangereuses (donc pas de risque d'injection SQL ici).
// =====================================================================
void init_db() {
    sqlite3 *db;
    char *err_msg = 0;
    char db_file_path[1024];
    
    // Get the proper database path from filesystem module
    if (!aura_fs_get_data_path("local.db", db_file_path, sizeof(db_file_path))) {
        fprintf(stderr, "Erreur: impossible d'obtenir le chemin de la BDD\n");
        return;
    }
    
    printf("[DB] Opening database at: %s\n", db_file_path);
    
    // Ouvre (ou crée) la base de données
    int rc = sqlite3_open(db_file_path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Erreur d'ouverture de la BDD: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }
    
    printf("[DB] Database opened successfully\n");
    
    // Création de la table si elle n'existe pas
    const char *sql = "CREATE TABLE IF NOT EXISTS interviews ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "user_name TEXT NOT NULL,"
                      "score INTEGER NOT NULL,"
                      "categorie TEXT);";
    
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK ) {
        fprintf(stderr, "Erreur SQL: %s\n", err_msg);
        sqlite3_free(err_msg);
    } else {
        printf("[DB] Table initialized successfully\n");
    }

    sqlite3_close(db);
    qb_init();
}

// Create users and verification tables
static void ensure_user_tables(sqlite3 *db) {
    char *err_msg = NULL;
    const char *sql_users =
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fullname TEXT NOT NULL,"
        "gmail TEXT NOT NULL UNIQUE,"
        "salt TEXT NOT NULL,"
        "hash TEXT NOT NULL,"
        "verified INTEGER NOT NULL DEFAULT 0,"
        "created_ts INTEGER NOT NULL DEFAULT (strftime('%s','now')),"
        "activated_ts INTEGER"
        ");";

    const char *sql_codes =
        "CREATE TABLE IF NOT EXISTS verification_codes ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "gmail TEXT NOT NULL,"
        "code_hash TEXT NOT NULL,"
        "code_salt TEXT NOT NULL,"
        "expiry_ts INTEGER NOT NULL,"
        "attempts_left INTEGER NOT NULL DEFAULT 5,"
        "resend_count INTEGER NOT NULL DEFAULT 0"
        ");";

    sqlite3_exec(db, sql_users, 0, 0, &err_msg);
    if (err_msg) { fprintf(stderr, "DB create users error: %s\n", err_msg); sqlite3_free(err_msg); err_msg = NULL; }
    sqlite3_exec(db, sql_codes, 0, 0, &err_msg);
    if (err_msg) { fprintf(stderr, "DB create codes error: %s\n", err_msg); sqlite3_free(err_msg); err_msg = NULL; }
}

bool db_create_user(const char *fullname, const char *gmail, const char *salt, const char *hash, char *errbuf, size_t errlen) {
    sqlite3 *db;
    char db_file_path[1024];
    if (!aura_fs_get_data_path("local.db", db_file_path, sizeof(db_file_path))) {
        if (errbuf) snprintf(errbuf, errlen, "Failed to get DB path");
        return false;
    }
    if (sqlite3_open(db_file_path, &db) != SQLITE_OK) {
        if (errbuf) snprintf(errbuf, errlen, "Failed to open DB");
        return false;
    }
    ensure_user_tables(db);
    const char *sql = "INSERT INTO users(fullname, gmail, salt, hash, verified) VALUES(?, ?, ?, ?, 1);";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        if (errbuf) snprintf(errbuf, errlen, "DB prepare error");
        sqlite3_close(db);
        return false;
    }
    sqlite3_bind_text(stmt, 1, fullname, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, gmail, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, salt, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, hash, -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    if (rc != SQLITE_DONE) {
        if (errbuf) snprintf(errbuf, errlen, "Failed to create user: %s", sqlite3_errmsg(db));
        return false;
    }
    return true;
}

bool db_is_duplicate_email(const char *gmail) {
    sqlite3 *db;
    char db_file_path[1024];
    if (!aura_fs_get_data_path("local.db", db_file_path, sizeof(db_file_path))) return false;
    if (sqlite3_open(db_file_path, &db) != SQLITE_OK) return false;
    ensure_user_tables(db);
    const char *sql = "SELECT COUNT(1) FROM users WHERE LOWER(gmail)=LOWER(?);";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    bool dup = false;
    if (rc == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, gmail, -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int cnt = sqlite3_column_int(stmt, 0);
            dup = (cnt > 0);
        }
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
    return dup;
}

bool db_store_verification_code(const char *gmail, const char *code_hash, const char *code_salt, long expiry_ts, int max_attempts, char *errbuf, size_t errlen) {
    sqlite3 *db;
    char db_file_path[1024];
    if (!aura_fs_get_data_path("local.db", db_file_path, sizeof(db_file_path))) {
        if (errbuf) snprintf(errbuf, errlen, "Failed to get DB path");
        return false;
    }
    if (sqlite3_open(db_file_path, &db) != SQLITE_OK) {
        if (errbuf) snprintf(errbuf, errlen, "Failed to open DB");
        return false;
    }
    ensure_user_tables(db);
    // remove any existing codes for this gmail
    const char *del = "DELETE FROM verification_codes WHERE LOWER(gmail)=LOWER(?);";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, del, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, gmail, -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    const char *sql = "INSERT INTO verification_codes(gmail, code_hash, code_salt, expiry_ts, attempts_left, resend_count) VALUES(?, ?, ?, ?, ?, 0);";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        if (errbuf) snprintf(errbuf, errlen, "DB prepare error");
        sqlite3_close(db);
        return false;
    }
    sqlite3_bind_text(stmt, 1, gmail, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, code_hash, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, code_salt, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, expiry_ts);
    sqlite3_bind_int(stmt, 5, max_attempts);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    if (rc != SQLITE_DONE) {
        if (errbuf) snprintf(errbuf, errlen, "Failed to store code");
        return false;
    }
    return true;
}

bool db_consume_verification_code(const char *gmail, const char *code, char *errbuf, size_t errlen) {
    sqlite3 *db;
    char db_file_path[1024];
    if (!aura_fs_get_data_path("local.db", db_file_path, sizeof(db_file_path))) { if (errbuf) snprintf(errbuf, errlen, "Failed to get DB path"); return false; }
    if (sqlite3_open(db_file_path, &db) != SQLITE_OK) { if (errbuf) snprintf(errbuf, errlen, "Failed to open DB"); return false; }
    ensure_user_tables(db);

    const char *sql = "SELECT id, code_hash, code_salt, expiry_ts, attempts_left FROM verification_codes WHERE LOWER(gmail)=LOWER(?);";
    sqlite3_stmt *stmt;
    bool ok = false;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, gmail, -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            const unsigned char *code_hash = sqlite3_column_text(stmt, 1);
            const unsigned char *code_salt = sqlite3_column_text(stmt, 2);
            long expiry = (long)sqlite3_column_int64(stmt, 3);
            int attempts_left = sqlite3_column_int(stmt, 4);

            long now = time(NULL);
            if (now > expiry) {
                sqlite3_finalize(stmt);
                // remove expired
                const char *del = "DELETE FROM verification_codes WHERE id=?;";
                sqlite3_stmt *d2;
                if (sqlite3_prepare_v2(db, del, -1, &d2, NULL) == SQLITE_OK) {
                    sqlite3_bind_int(d2, 1, id);
                    sqlite3_step(d2);
                    sqlite3_finalize(d2);
                }
                if (errbuf) snprintf(errbuf, errlen, "Code expired");
                sqlite3_close(db);
                return false;
            }

            if (attempts_left <= 0) {
                sqlite3_finalize(stmt);
                if (errbuf) snprintf(errbuf, errlen, "Too many attempts");
                sqlite3_close(db);
                return false;
            }

            // compute hash of provided code + salt
            GChecksum *chk = g_checksum_new(G_CHECKSUM_SHA256);
            g_checksum_update(chk, (const guchar *)code_salt, strlen((const char*)code_salt));
            g_checksum_update(chk, (const guchar *)code, strlen(code));
            const gchar *digest = g_checksum_get_string(chk);
            gchar local_digest[129]; strncpy(local_digest, digest, sizeof(local_digest)-1); local_digest[128]='\0';
            g_checksum_free(chk);

            if (g_strcmp0(local_digest, (const char*)code_hash) == 0) {
                ok = true;
                // consume code and delete
                sqlite3_finalize(stmt);
                const char *del = "DELETE FROM verification_codes WHERE id=?;";
                sqlite3_stmt *d2;
                if (sqlite3_prepare_v2(db, del, -1, &d2, NULL) == SQLITE_OK) {
                    sqlite3_bind_int(d2, 1, id);
                    sqlite3_step(d2);
                    sqlite3_finalize(d2);
                }
                // mark user verified
                const char *upd = "UPDATE users SET verified=1, activated_ts=strftime('%s','now') WHERE LOWER(gmail)=LOWER(?);";
                sqlite3_stmt *u2;
                if (sqlite3_prepare_v2(db, upd, -1, &u2, NULL) == SQLITE_OK) {
                    sqlite3_bind_text(u2, 1, gmail, -1, SQLITE_TRANSIENT);
                    sqlite3_step(u2);
                    sqlite3_finalize(u2);
                }
            } else {
                // decrement attempts_left
                sqlite3_finalize(stmt);
                const char *upd = "UPDATE verification_codes SET attempts_left = attempts_left - 1 WHERE LOWER(gmail)=LOWER(?);";
                sqlite3_stmt *u2;
                if (sqlite3_prepare_v2(db, upd, -1, &u2, NULL) == SQLITE_OK) {
                    sqlite3_bind_text(u2, 1, gmail, -1, SQLITE_TRANSIENT);
                    sqlite3_step(u2);
                    sqlite3_finalize(u2);
                }
                if (errbuf) snprintf(errbuf, errlen, "Invalid code");
                sqlite3_close(db);
                return false;
            }
        } else {
            if (errbuf) snprintf(errbuf, errlen, "No pending code");
        }
        sqlite3_finalize(stmt);
    } else {
        if (errbuf) snprintf(errbuf, errlen, "DB lookup error");
    }
    sqlite3_close(db);
    return ok;
}

bool db_verify_user_credentials(const char *email, const char *password,
    char *fullname_out, size_t fullname_len, int *verified_out,
    char *salt_out, size_t salt_len, char *hash_out, size_t hash_len) {
    sqlite3 *db;
    char db_file_path[1024];
    bool ok = false;

    if (!email || !password) return false;

    if (!aura_fs_get_data_path("local.db", db_file_path, sizeof(db_file_path))) return false;
    if (sqlite3_open(db_file_path, &db) != SQLITE_OK) return false;

    ensure_user_tables(db);

    const char *sql =
        "SELECT fullname, salt, hash, verified FROM users WHERE LOWER(gmail)=LOWER(?);";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, email, -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char *fullname = sqlite3_column_text(stmt, 0);
            const unsigned char *salt = sqlite3_column_text(stmt, 1);
            const unsigned char *hash = sqlite3_column_text(stmt, 2);
            int verified = sqlite3_column_int(stmt, 3);

            if (fullname_out && fullname_len > 0 && fullname) {
                strncpy(fullname_out, (const char *)fullname, fullname_len - 1);
                fullname_out[fullname_len - 1] = '\0';
            }
            if (salt_out && salt_len > 0 && salt) {
                strncpy(salt_out, (const char *)salt, salt_len - 1);
                salt_out[salt_len - 1] = '\0';
            }
            if (hash_out && hash_len > 0 && hash) {
                strncpy(hash_out, (const char *)hash, hash_len - 1);
                hash_out[hash_len - 1] = '\0';
            }
            if (verified_out) {
                *verified_out = verified;
            }
            ok = true;
        }
        sqlite3_finalize(stmt);
    }

    sqlite3_close(db);
    (void)password;
    return ok;
}

bool db_activate_user(const char *gmail, char *errbuf, size_t errlen) {
    sqlite3 *db;
    char db_file_path[1024];
    if (!aura_fs_get_data_path("local.db", db_file_path, sizeof(db_file_path))) { if (errbuf) snprintf(errbuf, errlen, "Failed to get DB path"); return false; }
    if (sqlite3_open(db_file_path, &db) != SQLITE_OK) { if (errbuf) snprintf(errbuf, errlen, "Failed to open DB"); return false; }
    ensure_user_tables(db);
    const char *upd = "UPDATE users SET verified=1, activated_ts=strftime('%s','now') WHERE LOWER(gmail)=LOWER(?);";
    sqlite3_stmt *stmt;
    bool ok = false;
    if (sqlite3_prepare_v2(db, upd, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, gmail, -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_DONE) ok = true;
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
    return ok;
}

// =====================================================================
// EXPLICATION SOUTENANCE : save_score() 
// =====================================================================
// Cette fonction protège contre les failles d'injections SQL grâce 
// aux "Prepared Statements" (Requêtes préparées).
// 
// Mécanisme :
// 1. `sqlite3_prepare_v2` : Analyse syntaxiquement la requête sans l'exécuter.
//    Ceci renvoie un objet "statement" (`sqlite3_stmt *stmt`).
// 2. `sqlite3_bind_*` : Sécurise et remplace les "?" de la requête par les 
//    vraies valeurs (le nom et le score). SQLite va échapper automatiquement
//    les caractères malveillants fournis dans l'interface utilisateur.
// 3. `sqlite3_step` : Exécute concrètement la requête verrouillée sur le disque.
// 4. `sqlite3_finalize(stmt)` : Supprime expressément l'objet statement local
//    en mémoire vive (indispensable pour éviter les memory leaks et les crashs).
// =====================================================================
void save_score(const char* user_name, int score, const char* categorie) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    char db_file_path[1024];
    
    // Get the proper database path from filesystem module
    if (!aura_fs_get_data_path("local.db", db_file_path, sizeof(db_file_path))) {
        fprintf(stderr, "Erreur: impossible d'obtenir le chemin de la BDD\n");
        return;
    }
    
    int rc = sqlite3_open(db_file_path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Erreur d'ouverture de la BDD: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }
    
    // Utilisation de requêtes préparées pour éviter les failles d'injection SQL
    const char *sql = "INSERT INTO interviews(user_name, score, categorie) VALUES(?, ?, ?);";
    
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        // Lier les variables (1 = user_name, 2 = score)
        sqlite3_bind_text(stmt, 1, user_name, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, score);
        sqlite3_bind_text(stmt, 3, categorie, -1, SQLITE_TRANSIENT);
        
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            fprintf(stderr, "Erreur lors de l'insertion: %s\n", sqlite3_errmsg(db));
        }
        
        // Libération IMPÉRATIVE de la mémoire du "statement"
        sqlite3_finalize(stmt);
    } else {
        fprintf(stderr, "Erreur de préparation SQL: %s\n", sqlite3_errmsg(db));
    }
    
    sqlite3_close(db);
}

// =====================================================================
// EXPLICATION SOUTENANCE : show_scores()
// =====================================================================
// Une requête "SELECT" permet de chercher des données écrites dans la base.
// 
// Mécanisme de boucle et extraction :
// - `sqlite3_step(stmt)` est appelée dans une boucle `while()`. 
//   Elle avancera de "ligne en ligne" (Row by Row) à chaque itération.
// - `sqlite3_column_int` et `sqlite3_column_text` extraient précisément
//   les données des colonnes respectives indexées (0, 1, 2 = id, user_name, score).
// - Le pointeur texte `(const unsigned char *name)` retourné pointe directement
//   dans le flux mémoire du "statement". Il ne doit exister aucune allocation manuelle ici,
//   mais ce pointeur devient obsolète juste après `sqlite3_finalize(stmt)`.
// =====================================================================
int db_count_interviews(const char *username) {
    sqlite3 *db;
    char db_file_path[1024];
    int count = 0;
    if (!aura_fs_get_data_path("local.db", db_file_path, sizeof(db_file_path))) return 0;
    if (sqlite3_open(db_file_path, &db) != SQLITE_OK) return 0;
    const char *sql = username && username[0]
        ? "SELECT COUNT(1) FROM interviews WHERE user_name=?"
        : "SELECT COUNT(1) FROM interviews";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (username && username[0]) sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) count = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
    return count;
}

float db_average_score(const char *username) {
    sqlite3 *db;
    char db_file_path[1024];
    float avg = 0.0f;
    if (!aura_fs_get_data_path("local.db", db_file_path, sizeof(db_file_path))) return 0.0f;
    if (sqlite3_open(db_file_path, &db) != SQLITE_OK) return 0.0f;
    const char *sql = username && username[0]
        ? "SELECT AVG(score) FROM interviews WHERE user_name=?"
        : "SELECT AVG(score) FROM interviews";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (username && username[0]) sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_type(stmt, 0) != SQLITE_NULL)
            avg = (float)sqlite3_column_double(stmt, 0);
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
    return avg;
}

int db_fetch_recent_scores(DbScoreRow *rows, int max_rows, const char *username) {
    sqlite3 *db;
    char db_file_path[1024];
    int n = 0;
    if (!rows || max_rows <= 0) return 0;
    if (!aura_fs_get_data_path("local.db", db_file_path, sizeof(db_file_path))) return 0;
    if (sqlite3_open(db_file_path, &db) != SQLITE_OK) return 0;
    const char *sql = username && username[0]
        ? "SELECT user_name, score, categorie FROM interviews WHERE user_name=? ORDER BY id DESC LIMIT ?"
        : "SELECT user_name, score, categorie FROM interviews ORDER BY id DESC LIMIT ?";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        int bind_idx = 1;
        if (username && username[0]) sqlite3_bind_text(stmt, bind_idx++, username, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, bind_idx, max_rows);
        while (sqlite3_step(stmt) == SQLITE_ROW && n < max_rows) {
            const unsigned char *name = sqlite3_column_text(stmt, 0);
            strncpy(rows[n].user_name, name ? (const char *)name : "", sizeof(rows[n].user_name) - 1);
            rows[n].score = sqlite3_column_int(stmt, 1);
            const unsigned char *cat = sqlite3_column_text(stmt, 2);
            strncpy(rows[n].categorie, cat ? (const char *)cat : "", sizeof(rows[n].categorie) - 1);
            n++;
        }
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
    return n;
}

void db_save_answer_detail(const char *user, int question_id, const char *domain, const char *level,
    int verdict, int score, const char *feedback) {
    sqlite3 *db;
    char db_file_path[1024];
    if (!user || !user[0]) return;
    if (!aura_fs_get_data_path("local.db", db_file_path, sizeof(db_file_path))) return;
    if (sqlite3_open(db_file_path, &db) != SQLITE_OK) return;
    const char *sql =
        "INSERT INTO answer_details(user_name, question_id, domain, level, verdict, score, feedback) "
        "VALUES(?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, user, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, question_id);
        sqlite3_bind_text(stmt, 3, domain ? domain : "", -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, level ? level : "", -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 5, verdict);
        sqlite3_bind_int(stmt, 6, score);
        sqlite3_bind_text(stmt, 7, feedback ? feedback : "", -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
}

int db_fetch_leaderboard_pro(DbLeaderboardRow *rows, int max_rows,
    const char *domain_filter, const char *level_filter) {
    sqlite3 *db;
    char db_file_path[1024];
    int n = 0;
    if (!rows || max_rows <= 0) return 0;
    if (!aura_fs_get_data_path("local.db", db_file_path, sizeof(db_file_path))) return 0;
    if (sqlite3_open(db_file_path, &db) != SQLITE_OK) return 0;

    char sql[1024];
    snprintf(sql, sizeof(sql),
        "SELECT user_name, CAST(AVG(score) AS INTEGER), MAX(score), COUNT(*), "
        "CASE WHEN instr(categorie, ' - ') > 0 "
        "THEN substr(categorie, 1, instr(categorie, ' - ') - 1) ELSE categorie END "
        "FROM interviews WHERE 1=1");
    if (domain_filter && domain_filter[0]) {
        char clause[128];
        snprintf(clause, sizeof(clause), " AND categorie LIKE '%%%s%%'", domain_filter);
        strncat(sql, clause, sizeof(sql) - strlen(sql) - 1);
    }
    if (level_filter && level_filter[0]) {
        char clause[128];
        snprintf(clause, sizeof(clause), " AND categorie LIKE '%% - %s'", level_filter);
        strncat(sql, clause, sizeof(sql) - strlen(sql) - 1);
    }
    strncat(sql, " GROUP BY user_name "
        "ORDER BY AVG(score) DESC, MAX(score) DESC, COUNT(*) DESC LIMIT ?;",
        sizeof(sql) - strlen(sql) - 1);

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, max_rows);
        while (sqlite3_step(stmt) == SQLITE_ROW && n < max_rows) {
            const unsigned char *name = sqlite3_column_text(stmt, 0);
            strncpy(rows[n].user_name, name ? (const char *)name : "", sizeof(rows[n].user_name) - 1);
            rows[n].avg_score = sqlite3_column_int(stmt, 1);
            rows[n].best_score = sqlite3_column_int(stmt, 2);
            rows[n].interviews = sqlite3_column_int(stmt, 3);
            const unsigned char *dom = sqlite3_column_text(stmt, 4);
            strncpy(rows[n].domain, dom ? (const char *)dom : "", sizeof(rows[n].domain) - 1);
            rows[n].rank = n + 1;
            n++;
        }
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
    return n;
}

int db_fetch_leaderboard(DbScoreRow *rows, int max_rows) {
    sqlite3 *db;
    char db_file_path[1024];
    int n = 0;
    if (!rows || max_rows <= 0) return 0;
    if (!aura_fs_get_data_path("local.db", db_file_path, sizeof(db_file_path))) return 0;
    if (sqlite3_open(db_file_path, &db) != SQLITE_OK) return 0;
    const char *sql =
        "SELECT user_name, CAST(AVG(score) AS INTEGER), categorie FROM interviews "
        "GROUP BY user_name, categorie ORDER BY AVG(score) DESC LIMIT ?";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, max_rows);
        while (sqlite3_step(stmt) == SQLITE_ROW && n < max_rows) {
            const unsigned char *name = sqlite3_column_text(stmt, 0);
            strncpy(rows[n].user_name, name ? (const char *)name : "", sizeof(rows[n].user_name) - 1);
            rows[n].score = sqlite3_column_int(stmt, 1);
            const unsigned char *cat = sqlite3_column_text(stmt, 2);
            strncpy(rows[n].categorie, cat ? (const char *)cat : "", sizeof(rows[n].categorie) - 1);
            n++;
        }
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
    return n;
}

void show_scores() {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    char db_file_path[1024];
    
    // Get the proper database path from filesystem module
    if (!aura_fs_get_data_path("local.db", db_file_path, sizeof(db_file_path))) {
        fprintf(stderr, "Erreur: impossible d'obtenir le chemin de la BDD\n");
        return;
    }
    
    int rc = sqlite3_open(db_file_path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Erreur d'ouverture de la BDD: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }
    
    printf("\n" "\x1b[36m" "============================================================" "\x1b[0m" "\n");
    printf(" | " "\x1b[33m" "ID" "\x1b[0m" "   | " "\x1b[33m" "UTILISATEUR" "\x1b[0m" "      | " "\x1b[33m" "SCORE" "\x1b[0m" " | " "\x1b[33m" "CATEGORIE" "\x1b[0m" "   |\n");
    printf("------------------------------------------------------------\n");
    
    const char* sql = "SELECT id, user_name, score, categorie FROM interviews;";
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    
    if (rc == SQLITE_OK) {
        // Boucle pour lire proprement chaque ligne retournée
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            const unsigned char *name = sqlite3_column_text(stmt, 1);
            int score = sqlite3_column_int(stmt, 2);
            const unsigned char *cat = sqlite3_column_text(stmt, 3);
            
            printf(" | %-4d | %-16s | %-5d | %-11s |\n", id, name ? (const char*)name : "INCONNU", score, cat ? (const char*)cat : "INCONNU");
        }
        
        // Libération IMPÉRATIVE de la mémoire du "statement"
        sqlite3_finalize(stmt);
    } else {
        fprintf(stderr, "Erreur SELECT SQL: %s\n", sqlite3_errmsg(db));
    }
    
    printf("\x1b[36m" "============================================================" "\x1b[0m" "\n");
    
    sqlite3_close(db);
}