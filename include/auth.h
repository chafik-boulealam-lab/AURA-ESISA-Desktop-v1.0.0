#ifndef AURA_AUTH_H
#define AURA_AUTH_H

#include <glib.h>
#include <stdbool.h>
#include <stddef.h>

#define AURA_MAX_NAME 128
#define AURA_MAX_EMAIL 128
#define AURA_MAX_PASSWORD 128

typedef struct {
    char fullname[AURA_MAX_NAME];
    char gmail[AURA_MAX_EMAIL];
    // stored hashed password fields
    char salt[64];
    char hash[128];
    int verified; /* 0 = not verified, 1 = verified */
} AuraAccount;

// Initialize auth subsystem (creates accounts file if missing)
bool auth_init(const char *accounts_path);

// Validate gmail; returns TRUE if valid, FALSE and fills errbuf otherwise
bool auth_validate_gmail(const char *gmail, char *errbuf, size_t errlen);

// Create account; returns TRUE on success, FALSE on failure and fills errbuf
bool auth_create_account(const char *fullname, const char *gmail, const char *password, char *errbuf, size_t errlen);

// Check credentials; returns TRUE if credentials match an existing account
bool auth_verify_credentials(const char *gmail, const char *password, AuraAccount *out_account);

// Check duplicate gmail; returns TRUE if gmail already exists
bool auth_is_duplicate(const char *gmail);

// Send a time-limited verification code to the given gmail.
// If `out_code` is non-NULL the generated 6-digit code is written there.
// Returns TRUE if the code was generated and stored (email may still fail).
bool auth_send_verification_code(const char *gmail, unsigned int *out_code, char *errbuf, size_t errlen);

// Verify a previously generated code for gmail (returns TRUE on success)
bool auth_verify_code(const char *gmail, const char *code, char *errbuf, size_t errlen);

// Path to accounts file (set by auth_init)
const char *auth_get_accounts_path(void);

#endif // AURA_AUTH_H
