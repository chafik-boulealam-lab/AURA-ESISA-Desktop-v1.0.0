#ifndef DB_H
#define DB_H

void init_db();
void save_score(const char* user_name, int score, const char* categorie);
void show_scores();

// User / verification helpers
bool db_create_user(const char *fullname, const char *gmail, const char *salt, const char *hash, char *errbuf, size_t errlen);
bool db_is_duplicate_email(const char *gmail);
bool db_store_verification_code(const char *gmail, const char *code_hash, const char *code_salt, long expiry_ts, int max_attempts, char *errbuf, size_t errlen);
// consume code and return true if ok
bool db_consume_verification_code(const char *gmail, const char *code, char *errbuf, size_t errlen);
bool db_activate_user(const char *gmail, char *errbuf, size_t errlen);
bool db_verify_user_credentials(const char *email, const char *password,
    char *fullname_out, size_t fullname_len, int *verified_out,
    char *salt_out, size_t salt_len, char *hash_out, size_t hash_len);

typedef struct {
    char user_name[128];
    int score;
    char categorie[128];
} DbScoreRow;

typedef struct {
    int rank;
    char user_name[128];
    int avg_score;
    int best_score;
    int interviews;
    char domain[128];
} DbLeaderboardRow;

int db_count_interviews(const char *username);
float db_average_score(const char *username);
int db_fetch_recent_scores(DbScoreRow *rows, int max_rows, const char *username);
int db_fetch_leaderboard(DbScoreRow *rows, int max_rows);
int db_fetch_leaderboard_pro(DbLeaderboardRow *rows, int max_rows,
    const char *domain_filter, const char *level_filter);
void db_save_answer_detail(const char *user, int question_id, const char *domain, const char *level,
    int verdict, int score, const char *feedback);

#endif // DB_H