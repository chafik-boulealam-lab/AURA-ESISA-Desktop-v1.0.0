#ifndef QUESTION_SEED_H
#define QUESTION_SEED_H

#include <sqlite3.h>

int qb_seed_domain_level(sqlite3 *db, const char *domain, const char *level, int target_count);
int qb_import_csv(sqlite3 *db, const char *csv_path);

#endif