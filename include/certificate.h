#ifndef CERTIFICATE_H
#define CERTIFICATE_H

#include "types.h"

bool certificate_generate_pdf(const char *username, const char *domain, const char *level, const char *date, float score, const char *out_path);

#endif // CERTIFICATE_H
