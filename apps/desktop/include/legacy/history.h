#ifndef HISTORY_H
#define HISTORY_H

#include "types.h"

void history_add_entry(const History *h);
History *history_list_all(size_t *out_count);
void history_free_list(History *list, size_t count);

#endif // HISTORY_H
