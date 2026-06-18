#ifndef QUESTIONS_H
#define QUESTIONS_H

#include "types.h"

typedef struct Question Question;

bool questions_load(const char *path);
bool questions_ensure_loaded(void);
int questions_count(void);
Question *questions_get_random(const char *domain, const char *level);
bool questions_pick_for_interview(const char *domain, const char *level,
    const int *exclude_ids, int exclude_count,
    int *out_id, char *out_text, size_t text_len,
    char *out_ref, size_t ref_len, const char *username);
void questions_free_all(void);

#endif // QUESTIONS_H
