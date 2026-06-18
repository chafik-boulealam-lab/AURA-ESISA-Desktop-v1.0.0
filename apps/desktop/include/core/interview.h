#ifndef INTERVIEW_H
#define INTERVIEW_H

#include "types.h"

typedef struct Interview Interview;

Interview *interview_create(const char *user, const char *domain, const char *level, int questions_count);
void interview_destroy(Interview *iv);
bool interview_next(Interview *iv, char *out_question, size_t qlen);
bool interview_prev(Interview *iv, char *out_question, size_t qlen);
void interview_record_answer(Interview *iv, int question_index, const char *answer);
int interview_finish(Interview *iv);
int interview_get_current_index(Interview *iv);
bool interview_get_current_question(Interview *iv, char *out_question, size_t qlen);
const char *interview_get_answer(Interview *iv, int question_index);
const char *interview_get_user(Interview *iv);
const char *interview_get_domain(Interview *iv);
const char *interview_get_level(Interview *iv);
int interview_get_questions_count(Interview *iv);
int interview_get_question_id(Interview *iv, int question_index);
const char *interview_get_reference(Interview *iv, int question_index);
int interview_get_verdict(Interview *iv, int question_index);
const char *interview_get_feedback(Interview *iv, int question_index);
int interview_evaluate_current(Interview *iv, int question_index);
bool interview_get_question_text(Interview *iv, int question_index, char *out, size_t len);
int interview_get_score(Interview *iv, int question_index);
int interview_compute_average(Interview *iv);

#endif // INTERVIEW_H
