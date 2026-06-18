#ifndef REPORT_H
#define REPORT_H

#include "interview.h"
#include <stddef.h>
#include <stdbool.h>

bool report_generate_session_pdf(Interview *iv, int avg_score, char *out_path, size_t path_len);
bool report_open_file(const char *path);
int report_list_pdfs(char paths[][1024], int max_paths);

#endif