#include "file_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool fm_save_history(const History *h, const char *path) {
    (void)h; (void)path;
    return false;
}

History *fm_load_history(const char *path, size_t *out_count) {
    (void)path; if (out_count) *out_count = 0; return NULL;
}
