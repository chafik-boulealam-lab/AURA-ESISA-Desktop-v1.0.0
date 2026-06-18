#include "db.h"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"

static sqlite3 *g_db = NULL;

static int is_blank(const char *s) {
    return s == NULL || s[0] == '\0';
}

int backend_db_init(const char *db_path, char *err, size_t errlen) {
    char *sql_err = NULL;
    const char *schema_users =
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "email TEXT NOT NULL UNIQUE,"
        "password TEXT NOT NULL"
        ");";
    const char *schema_tasks =
        "CREATE TABLE IF NOT EXISTS tasks ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_email TEXT NOT NULL,"
        "title TEXT NOT NULL,"
        "done INTEGER NOT NULL DEFAULT 0"
        ");";
    const char *schema_notes =
        "CREATE TABLE IF NOT EXISTS notes ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_email TEXT NOT NULL,"
        "content TEXT NOT NULL"
        ");";

    if (g_db != NULL) {
        return 1;
    }

    if (sqlite3_open(db_path, &g_db) != SQLITE_OK) {
        if (err) {
            snprintf(err, errlen, "Failed to open sqlite database: %s", sqlite3_errmsg(g_db));
        }
        if (g_db) {
            sqlite3_close(g_db);
            g_db = NULL;
        }
        return 0;
    }

    if (sqlite3_exec(g_db, schema_users, NULL, NULL, &sql_err) != SQLITE_OK) {
        if (err) snprintf(err, errlen, "Schema error(users): %s", sql_err ? sql_err : "unknown");
        sqlite3_free(sql_err);
        return 0;
    }
    if (sqlite3_exec(g_db, schema_tasks, NULL, NULL, &sql_err) != SQLITE_OK) {
        if (err) snprintf(err, errlen, "Schema error(tasks): %s", sql_err ? sql_err : "unknown");
        sqlite3_free(sql_err);
        return 0;
    }
    if (sqlite3_exec(g_db, schema_notes, NULL, NULL, &sql_err) != SQLITE_OK) {
        if (err) snprintf(err, errlen, "Schema error(notes): %s", sql_err ? sql_err : "unknown");
        sqlite3_free(sql_err);
        return 0;
    }

    return 1;
}

int backend_register_user(const char *email, const char *password, char *err, size_t errlen) {
    sqlite3_stmt *stmt = NULL;
    const char *sql = "INSERT INTO users(email, password) VALUES(?, ?);";

    if (is_blank(email) || is_blank(password)) {
        if (err) snprintf(err, errlen, "email and password are required");
        return 0;
    }

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        if (err) snprintf(err, errlen, "prepare failed: %s", sqlite3_errmsg(g_db));
        return 0;
    }

    sqlite3_bind_text(stmt, 1, email, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        if (err) snprintf(err, errlen, "register failed: %s", sqlite3_errmsg(g_db));
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return 1;
}

int backend_login_user(const char *email, const char *password, char *err, size_t errlen) {
    sqlite3_stmt *stmt = NULL;
    const char *sql = "SELECT id FROM users WHERE email = ? AND password = ?;";
    int rc;

    if (is_blank(email) || is_blank(password)) {
        if (err) snprintf(err, errlen, "email and password are required");
        return 0;
    }

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        if (err) snprintf(err, errlen, "prepare failed: %s", sqlite3_errmsg(g_db));
        return 0;
    }

    sqlite3_bind_text(stmt, 1, email, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_ROW) {
        return 1;
    }

    if (err) snprintf(err, errlen, "invalid credentials");
    return 0;
}

int backend_add_task(const char *email, const char *title, char *err, size_t errlen) {
    sqlite3_stmt *stmt = NULL;
    const char *sql = "INSERT INTO tasks(user_email, title, done) VALUES(?, ?, 0);";

    if (is_blank(email) || is_blank(title)) {
        if (err) snprintf(err, errlen, "email and title are required");
        return 0;
    }

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        if (err) snprintf(err, errlen, "prepare failed: %s", sqlite3_errmsg(g_db));
        return 0;
    }

    sqlite3_bind_text(stmt, 1, email, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, title, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        if (err) snprintf(err, errlen, "insert task failed: %s", sqlite3_errmsg(g_db));
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return 1;
}

char *backend_list_tasks_json(const char *email, char *err, size_t errlen) {
    sqlite3_stmt *stmt = NULL;
    const char *sql = "SELECT id, title, done FROM tasks WHERE user_email = ? ORDER BY id DESC;";
    cJSON *arr = NULL;
    char *out = NULL;

    if (is_blank(email)) {
        if (err) snprintf(err, errlen, "email is required");
        return NULL;
    }

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        if (err) snprintf(err, errlen, "prepare failed: %s", sqlite3_errmsg(g_db));
        return NULL;
    }

    sqlite3_bind_text(stmt, 1, email, -1, SQLITE_TRANSIENT);
    arr = cJSON_CreateArray();

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        cJSON *obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(obj, "id", sqlite3_column_int(stmt, 0));
        cJSON_AddStringToObject(obj, "title", (const char *)sqlite3_column_text(stmt, 1));
        cJSON_AddNumberToObject(obj, "done", sqlite3_column_int(stmt, 2));
        cJSON_AddItemToArray(arr, obj);
    }

    sqlite3_finalize(stmt);

    out = cJSON_PrintUnformatted(arr);
    cJSON_Delete(arr);
    return out;
}

int backend_add_note(const char *email, const char *content, char *err, size_t errlen) {
    sqlite3_stmt *stmt = NULL;
    const char *sql = "INSERT INTO notes(user_email, content) VALUES(?, ?);";

    if (is_blank(email) || is_blank(content)) {
        if (err) snprintf(err, errlen, "email and content are required");
        return 0;
    }

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        if (err) snprintf(err, errlen, "prepare failed: %s", sqlite3_errmsg(g_db));
        return 0;
    }

    sqlite3_bind_text(stmt, 1, email, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, content, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        if (err) snprintf(err, errlen, "insert note failed: %s", sqlite3_errmsg(g_db));
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return 1;
}

char *backend_list_notes_json(const char *email, char *err, size_t errlen) {
    sqlite3_stmt *stmt = NULL;
    const char *sql = "SELECT id, content FROM notes WHERE user_email = ? ORDER BY id DESC;";
    cJSON *arr = NULL;
    char *out = NULL;

    if (is_blank(email)) {
        if (err) snprintf(err, errlen, "email is required");
        return NULL;
    }

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        if (err) snprintf(err, errlen, "prepare failed: %s", sqlite3_errmsg(g_db));
        return NULL;
    }

    sqlite3_bind_text(stmt, 1, email, -1, SQLITE_TRANSIENT);
    arr = cJSON_CreateArray();

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        cJSON *obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(obj, "id", sqlite3_column_int(stmt, 0));
        cJSON_AddStringToObject(obj, "content", (const char *)sqlite3_column_text(stmt, 1));
        cJSON_AddItemToArray(arr, obj);
    }

    sqlite3_finalize(stmt);

    out = cJSON_PrintUnformatted(arr);
    cJSON_Delete(arr);
    return out;
}
