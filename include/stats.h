#ifndef STATS_H
#define STATS_H

#include "types.h"

void stats_record_interview(const History *h);
Statistics stats_get_user_stats(const char *username);

#endif // STATS_H
