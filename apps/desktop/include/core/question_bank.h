#ifndef QUESTION_BANK_H
#define QUESTION_BANK_H

#include <stddef.h>
#include <stdbool.h>

#define QB_MIN_PER_LEVEL 400

bool qb_init(void);
int qb_count(const char *domain, const char *level);
bool qb_pick_unique(const char *domain, const char *level,
    const int *exclude_ids, int exclude_count,
    int *out_id, char *out_text, size_t text_len,
    char *out_ref, size_t ref_len,
    const char *username);
void qb_mark_answered(const char *username, int question_id);

#endif