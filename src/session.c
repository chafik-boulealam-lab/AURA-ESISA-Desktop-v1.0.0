#include "session.h"
#include <string.h>

AuraSession g_aura_session = {0};

void aura_session_set(const char *fullname, const char *email) {
    if (fullname) {
        strncpy(g_aura_session.fullname, fullname, sizeof(g_aura_session.fullname) - 1);
        g_aura_session.fullname[sizeof(g_aura_session.fullname) - 1] = '\0';
    }
    if (email) {
        strncpy(g_aura_session.email, email, sizeof(g_aura_session.email) - 1);
        g_aura_session.email[sizeof(g_aura_session.email) - 1] = '\0';
    }
    g_aura_session.authenticated = true;
}

void aura_session_clear(void) {
    memset(&g_aura_session, 0, sizeof(g_aura_session));
}

const char *aura_session_username(void) {
    if (g_aura_session.email[0] != '\0') return g_aura_session.email;
    if (g_aura_session.fullname[0] != '\0') return g_aura_session.fullname;
    return "etudiant";
}