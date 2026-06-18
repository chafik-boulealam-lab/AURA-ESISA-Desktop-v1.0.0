#ifndef AURA_SESSION_H
#define AURA_SESSION_H

#include <stdbool.h>

typedef struct {
    char fullname[128];
    char email[128];
    bool authenticated;
} AuraSession;

extern AuraSession g_aura_session;

void aura_session_set(const char *fullname, const char *email);
void aura_session_clear(void);
const char *aura_session_username(void);

#endif