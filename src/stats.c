#include "stats.h"
#include "db.h"
#include <string.h>

void stats_record_interview(const History *h) {
    if (!h) return;
    save_score(h->username, (int)h->score, h->domain);
}

Statistics stats_get_user_stats(const char *username) {
    Statistics s;
    memset(&s, 0, sizeof(s));
    s.interviews_taken = db_count_interviews(username);
    s.average_score = db_average_score(username);
    return s;
}