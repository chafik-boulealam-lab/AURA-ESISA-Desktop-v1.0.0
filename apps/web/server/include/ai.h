#ifndef BACKEND_AI_H
#define BACKEND_AI_H

#include <stddef.h>

int backend_ai_prompt(const char *prompt, char **out_reply, char *err, size_t errlen);

#endif
