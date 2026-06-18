#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "types.h"

bool fm_save_history(const History *h, const char *path);
History *fm_load_history(const char *path, size_t *out_count);

#endif // FILE_MANAGER_H
