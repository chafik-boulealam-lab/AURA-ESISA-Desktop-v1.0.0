#include "history.h"
#include <stdlib.h>
#include <string.h>

void history_add_entry(const History *h) {
    (void)h;
}

History *history_list_all(size_t *out_count) {
    if (out_count) *out_count = 0;
    return NULL;
}

void history_free_list(History *list, size_t count) {
    (void)list; (void)count;
}
