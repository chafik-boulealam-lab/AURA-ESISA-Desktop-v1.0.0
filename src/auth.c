#include "../include/auth.h"
#include "../include/db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdint.h>
#include <glib.h>
#include <curl/curl.h>

static char accounts_file_path[1024] = "bin/data/accounts.txt"; // legacy

typedef struct {
    const char *data;
    size_t len;
    size_t sent;
} AuraSmtpPayload;

static void trim_newline(char *s);
static bool gmail_matches(const char *lhs, const char *rhs);
static char *my_strsep(char **stringp, const char *delim);

static size_t smtp_payload_read_cb(char *ptr, size_t size, size_t nmemb, void *userdata) {
    AuraSmtpPayload *payload = (AuraSmtpPayload *)userdata;
    size_t max_bytes = size * nmemb;
    size_t remaining;
    size_t to_copy;

    if (!payload || !payload->data) {
        return 0;
    }

    remaining = payload->len - payload->sent;
    if (remaining == 0) {
        return 0;
    }

    to_copy = remaining < max_bytes ? remaining : max_bytes;
    memcpy(ptr, payload->data + payload->sent, to_copy);
    payload->sent += to_copy;
    return to_copy;
}

// verification codes are stored in SQLite via db_store_verification_code

static bool send_verification_code_email(const char *gmail, guint code, char *errbuf, size_t errlen) {
    const char *smtp_url = getenv("AURA_SMTP_URL");
    const char *smtp_user = getenv("AURA_SMTP_USERNAME");
    const char *smtp_pass = getenv("AURA_SMTP_PASSWORD");
    const char *smtp_from = getenv("AURA_SMTP_FROM");
    char from_addr[256];
    char code_str[16];
    char body[4096];
    AuraSmtpPayload payload;
    CURL *curl;
    CURLcode rc;
    struct curl_slist *recipients = NULL;

    if (!smtp_url || !smtp_user || !smtp_pass) {
        if (errbuf) {
            snprintf(
                errbuf,
                errlen,
                "Email delivery is not configured. Set AURA_SMTP_URL, AURA_SMTP_USERNAME, AURA_SMTP_PASSWORD."
            );
        }
        return false;
    }

    snprintf(from_addr, sizeof(from_addr), "%s", (smtp_from && smtp_from[0] != '\0') ? smtp_from : smtp_user);
    snprintf(code_str, sizeof(code_str), "%06u", code);

    /* HTML email template with professional branding */
    snprintf(
        body,
        sizeof(body),
        "To: %s\r\n"
        "From: %s\r\n"
        "Subject: AURA Verification Code\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n"
        "MIME-Version: 1.0\r\n"
        "\r\n"
        "<!DOCTYPE html>\r\n"
        "<html>\r\n"
        "<head>\r\n"
        "  <meta charset='UTF-8'>\r\n"
        "  <meta name='viewport' content='width=device-width, initial-scale=1.0'>\r\n"
        "  <style>\r\n"
        "    body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif; background: #0f172a; color: #e2e8f0; margin: 0; padding: 20px; }\r\n"
        "    .container { max-width: 600px; margin: 0 auto; background: #1e293b; border-radius: 8px; overflow: hidden; box-shadow: 0 4px 20px rgba(0,0,0,0.3); }\r\n"
        "    .header { background: linear-gradient(135deg, #00d9ff 0%%, #0084ff 100%%); padding: 40px 20px; text-align: center; }\r\n"
        "    .logo { font-size: 28px; font-weight: 900; color: #ffffff; margin: 0; letter-spacing: -0.5px; }\r\n"
        "    .content { padding: 40px 30px; }\r\n"
        "    .greeting { font-size: 18px; margin-bottom: 20px; color: #00d9ff; font-weight: 600; }\r\n"
        "    .message { font-size: 14px; line-height: 1.6; color: #cbd5e1; margin-bottom: 30px; }\r\n"
        "    .code-box { background: #0f172a; border: 2px solid #00d9ff; border-radius: 6px; padding: 20px; text-align: center; margin: 30px 0; }\r\n"
        "    .code { font-size: 36px; font-weight: 900; letter-spacing: 4px; color: #00d9ff; font-family: 'Courier New', monospace; }\r\n"
        "    .expiry { font-size: 13px; color: #94a3b8; margin-top: 20px; background: #1e293b; padding: 12px; border-radius: 4px; border-left: 3px solid #f97316; }\r\n"
        "    .footer { background: #0f172a; padding: 20px 30px; border-top: 1px solid #334155; text-align: center; font-size: 12px; color: #64748b; }\r\n"
        "    .footer-link { color: #00d9ff; text-decoration: none; }\r\n"
        "  </style>\r\n"
        "</head>\r\n"
        "<body>\r\n"
        "  <div class='container'>\r\n"
        "    <div class='header'>\r\n"
        "      <p class='logo'>⚡ AURA</p>\r\n"
        "    </div>\r\n"
        "    <div class='content'>\r\n"
        "      <p class='greeting'>Welcome to AURA</p>\r\n"
        "      <p class='message'>We received a request to verify your email address. Use the verification code below to complete your registration:</p>\r\n"
        "      <div class='code-box'>\r\n"
        "        <div class='code'>%s</div>\r\n"
        "      </div>\r\n"
        "      <p class='message'>This code is valid for <strong>10 minutes</strong> and can only be used once.</p>\r\n"
        "      <div class='expiry'>\r\n"
        "        <strong>Security Note:</strong> Never share this code with anyone. AURA support will never ask for your verification code.\r\n"
        "      </div>\r\n"
        "      <p class='message' style='margin-top: 30px; color: #94a3b8; font-size: 13px;'>\r\n"
        "        If you did not request this verification code, please ignore this email or contact our support team.\r\n"
        "      </p>\r\n"
        "    </div>\r\n"
        "    <div class='footer'>\r\n"
        "      <p>© 2026 AURA AI Training Environment. All rights reserved.</p>\r\n"
        "      <p>For support, contact <a href='mailto:support@aura.ai' class='footer-link'>support@aura.ai</a></p>\r\n"
        "    </div>\r\n"
        "  </div>\r\n"
        "</body>\r\n"
        "</html>\r\n",
        gmail,
        from_addr,
        code_str
    );

    payload.data = body;
    payload.len = strlen(body);
    payload.sent = 0;

    curl = curl_easy_init();
    if (!curl) {
        if (errbuf) snprintf(errbuf, errlen, "Failed to initialize email transport");
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, smtp_url);
    curl_easy_setopt(curl, CURLOPT_USERNAME, smtp_user);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, smtp_pass);
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from_addr);
    recipients = curl_slist_append(recipients, gmail);
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, smtp_payload_read_cb);
    curl_easy_setopt(curl, CURLOPT_READDATA, &payload);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);

    rc = curl_easy_perform(curl);
    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK) {
        if (errbuf) {
            snprintf(errbuf, errlen, "Email send failed: %s", curl_easy_strerror(rc));
        }
        return false;
    }

    return true;
}

bool auth_init(const char *accounts_path) {
    // initialize database and ensure user/code tables exist
    init_db();
    return true;
}

const char *auth_get_accounts_path(void) {
    return accounts_file_path;
}

static void trim_newline(char *s) {
    size_t l = strlen(s);
    while (l > 0 && (s[l-1] == '\n' || s[l-1] == '\r')) { s[--l] = '\0'; }
}

static bool gmail_matches(const char *lhs, const char *rhs) {
    return g_ascii_strcasecmp(lhs, rhs) == 0;
}

static void generate_salt(char *out, size_t outlen) {
    const char *chars = "0123456789abcdef";
    srand((unsigned int)time(NULL) ^ (unsigned int)((uintptr_t)out));
    for (size_t i = 0; i < outlen-1; i++) {
        out[i] = chars[rand() % 16];
    }
    out[outlen-1] = '\0';
}

// portable replacement for strsep
static char *my_strsep(char **stringp, const char *delim) {
    if (!stringp || !*stringp) return NULL;
    char *s = *stringp;
    char *p = s;
    while (*p && !strchr(delim, *p)) p++;
    if (*p) {
        *p = '\0';
        *stringp = p + 1;
    } else {
        *stringp = NULL;
    }
    return s;
}

// simple iterative SHA256 hash: hash = SHA256(hash || password)
static void hash_password(const char *salt, const char *password, char *out_hex, size_t out_hex_len) {
    GChecksum *chk = g_checksum_new(G_CHECKSUM_SHA256);
    // initial input: salt + password
    g_checksum_update(chk, (const guchar *)salt, strlen(salt));
    g_checksum_update(chk, (const guchar *)password, strlen(password));
    const gchar *digest = g_checksum_get_string(chk);
    // iterate to slow down
    char buf[65];
    strncpy(buf, digest, sizeof(buf)-1);
    buf[64] = '\0';
    for (int i = 0; i < 2000; i++) {
        g_checksum_reset(chk);
        g_checksum_update(chk, (const guchar *)buf, strlen(buf));
        const gchar *d2 = g_checksum_get_string(chk);
        strncpy(buf, d2, sizeof(buf)-1);
        buf[64] = '\0';
    }
    // copy final digest
    g_strlcpy(out_hex, buf, out_hex_len);
    g_checksum_free(chk);
}

bool auth_validate_gmail(const char *gmail, char *errbuf, size_t errlen) {
    if (!gmail || gmail[0] == '\0') {
        if (errbuf) snprintf(errbuf, errlen, "Email must not be empty");
        return false;
    }
    size_t len = strlen(gmail);
    if (len < 5) {
        if (errbuf) snprintf(errbuf, errlen, "Email too short");
        return false;
    }
    if (strchr(gmail, ' ')) {
        if (errbuf) snprintf(errbuf, errlen, "Email must not contain spaces");
        return false;
    }
    const char *at = strchr(gmail, '@');
    if (!at || at == gmail) {
        if (errbuf) snprintf(errbuf, errlen, "Email must contain a local part and domain");
        return false;
    }
    const char *domain = at + 1;
    if (domain[0] == '\0' || strchr(domain, '@') != NULL) {
        if (errbuf) snprintf(errbuf, errlen, "Email must contain a valid domain");
        return false;
    }
    if (!strchr(domain, '.')) {
        if (errbuf) snprintf(errbuf, errlen, "Email must contain a valid domain");
        return false;
    }
    return true;
}

bool auth_is_duplicate(const char *gmail) {
    if (!gmail) return false;
    return db_is_duplicate_email(gmail);
}

bool auth_create_account(const char *fullname, const char *gmail, const char *password, char *errbuf, size_t errlen) {
    if (!fullname || !gmail || !password) {
        if (errbuf) snprintf(errbuf, errlen, "Missing fields");
        return false;
    }
    if (!auth_validate_gmail(gmail, errbuf, errlen)) return false;
    if (strlen(password) < 6) {
        if (errbuf) snprintf(errbuf, errlen, "Password must be at least 6 characters");
        return false;
    }
    if (strchr(fullname, ',') || strchr(gmail, ',') || strchr(password, ',')) {
        if (errbuf) snprintf(errbuf, errlen, "Fields must not contain commas");
        return false;
    }
    if (auth_is_duplicate(gmail)) {
        if (errbuf) snprintf(errbuf, errlen, "An account with this email already exists");
        return false;
    }
    // generate salt and hashed password
    char salt[33];
    generate_salt(salt, sizeof(salt));
    char hash[65];
    hash_password(salt, password, hash, sizeof(hash));

    if (!db_create_user(fullname, gmail, salt, hash, errbuf, errlen)) {
        return false;
    }
    db_activate_user(gmail, NULL, 0);
    return true;
}

bool auth_verify_credentials(const char *gmail, const char *password, AuraAccount *out_account) {
    if (!gmail || !password) return false;

    char fullname[AURA_MAX_NAME];
    char salt[64];
    char hash[128];
    int verified = 0;
    if (db_verify_user_credentials(gmail, password, fullname, sizeof(fullname), &verified, salt, sizeof(salt), hash, sizeof(hash))) {
        char computed[65];
        hash_password(salt, password, computed, sizeof(computed));
        if (strcmp(computed, hash) == 0) {
            if (out_account) {
                strncpy(out_account->fullname, fullname, AURA_MAX_NAME - 1);
                out_account->fullname[AURA_MAX_NAME - 1] = '\0';
                strncpy(out_account->gmail, gmail, AURA_MAX_EMAIL - 1);
                out_account->gmail[AURA_MAX_EMAIL - 1] = '\0';
                strncpy(out_account->salt, salt, sizeof(out_account->salt) - 1);
                out_account->salt[sizeof(out_account->salt) - 1] = '\0';
                strncpy(out_account->hash, hash, sizeof(out_account->hash) - 1);
                out_account->hash[sizeof(out_account->hash) - 1] = '\0';
                out_account->verified = 1;
            }
            return true;
        }
    }

    FILE *f = fopen(accounts_file_path, "r");
    if (!f) return false;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        trim_newline(line);
        // parse fields (support legacy and new formats)
        char *save = line;
        char *fullname = my_strsep(&save, ",");
        char *email = my_strsep(&save, ",");
        char *field3 = my_strsep(&save, ",");
        if (!email) continue;

        if (!field3) continue;
        // legacy case: field3 is plaintext password
        if (save == NULL) {
            char *pw = field3;
            if (gmail_matches(email, gmail) && strcmp(pw, password) == 0) {
                if (out_account) {
                    strncpy(out_account->fullname, fullname, AURA_MAX_NAME-1);
                    out_account->fullname[AURA_MAX_NAME-1] = '\0';
                    strncpy(out_account->gmail, email, AURA_MAX_EMAIL-1);
                    out_account->gmail[AURA_MAX_EMAIL-1] = '\0';
                    out_account->verified = 1; // legacy accounts consider verified
                }
                fclose(f);
                return true;
            }
        } else {
            // new format: salt,hash,verified
            char *salt = field3;
            char *hash = my_strsep(&save, ",");
            char *verified_str = my_strsep(&save, ",");
            int verified = (verified_str && atoi(verified_str) != 0) ? 1 : 0;
            if (gmail_matches(email, gmail)) {
                char computed[65];
                hash_password(salt, password, computed, sizeof(computed));
                if (strcmp(computed, hash) == 0) {
                    if (out_account) {
                        strncpy(out_account->fullname, fullname, AURA_MAX_NAME-1);
                        out_account->fullname[AURA_MAX_NAME-1] = '\0';
                        strncpy(out_account->gmail, email, AURA_MAX_EMAIL-1);
                        out_account->gmail[AURA_MAX_EMAIL-1] = '\0';
                        strncpy(out_account->salt, salt, sizeof(out_account->salt)-1);
                        out_account->salt[sizeof(out_account->salt)-1] = '\0';
                        strncpy(out_account->hash, hash, sizeof(out_account->hash)-1);
                        out_account->hash[sizeof(out_account->hash)-1] = '\0';
                        out_account->verified = verified;
                    }
                    fclose(f);
                    return true;
                }
            }
        }
    }
    fclose(f);
    return false;
}

// Legacy function - no longer needed; account verification now handled by DB

bool auth_send_verification_code(const char *gmail, unsigned int *out_code, char *errbuf, size_t errlen) {
    if (!gmail) {
        if (errbuf) snprintf(errbuf, errlen, "Missing gmail");
        return false;
    }

    if (!auth_validate_gmail(gmail, errbuf, errlen)) {
        return false;
    }

    /* generate 6-digit code */
    guint code = (g_random_int() % 900000) + 100000;
    time_t expiry = time(NULL) + 600; // 10 minutes

    /* compute salt + hash for code and store in DB */
    char code_salt[33];
    generate_salt(code_salt, sizeof(code_salt));
    gchar code_str[16]; snprintf(code_str, sizeof(code_str), "%06u", code);
    GChecksum *chk = g_checksum_new(G_CHECKSUM_SHA256);
    g_checksum_update(chk, (const guchar *)code_salt, strlen(code_salt));
    g_checksum_update(chk, (const guchar *)code_str, strlen(code_str));
    const gchar *digest = g_checksum_get_string(chk);
    char code_hash[129]; strncpy(code_hash, digest, sizeof(code_hash)-1); code_hash[128] = '\0';
    g_checksum_free(chk);

    if (!db_store_verification_code(gmail, code_hash, code_salt, (long)expiry, 5, errbuf, errlen)) {
        if (errbuf) snprintf(errbuf, errlen, "Failed to store verification code");
        return false;
    }

    /* Send email via SMTP. If sending fails, return false. */
    if (!send_verification_code_email(gmail, code, errbuf, errlen)) {
        return false;
    }

    /* Only expose the code to the caller in explicit dev mode */
    const char *dev_show = getenv("AURA_DEV_SHOW_CODES");
    if (out_code && dev_show && atoi(dev_show) != 0) *out_code = (unsigned int)code;

    fprintf(stderr, "[AUTH] Verification code emailed to %s (expires in 10m)\n", gmail);
    return true;
}

bool auth_verify_code(const char *gmail, const char *code, char *errbuf, size_t errlen) {
    if (!gmail || !code) { if (errbuf) snprintf(errbuf, errlen, "Missing parameters"); return false; }
    if (!db_consume_verification_code(gmail, code, errbuf, errlen)) {
        return false;
    }
    return true;
}
