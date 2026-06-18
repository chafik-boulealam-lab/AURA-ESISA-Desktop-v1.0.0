#ifndef BACKEND_DB_H
#define BACKEND_DB_H

#include <stddef.h>

int backend_db_init(const char *db_path, char *err, size_t errlen);
int backend_register_user(const char *email, const char *password, char *err, size_t errlen);
int backend_login_user(const char *email, const char *password, char *err, size_t errlen);
int backend_add_task(const char *email, const char *title, char *err, size_t errlen);
char *backend_list_tasks_json(const char *email, char *err, size_t errlen);
int backend_add_note(const char *email, const char *content, char *err, size_t errlen);
char *backend_list_notes_json(const char *email, char *err, size_t errlen);

#endif
