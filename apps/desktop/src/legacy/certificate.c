#include "certificate.h"
#include <stdio.h>

bool certificate_generate_pdf(const char *username, const char *domain, const char *level, const char *date, float score, const char *out_path) {
    FILE *f = fopen(out_path, "w");
    if (!f) return false;
    fprintf(f, "Certificate for %s\nDomain: %s\nLevel: %s\nDate: %s\nScore: %.2f\n", username, domain, level, date, score);
    fclose(f);
    return true;
}
