#ifndef EVALUATION_H
#define EVALUATION_H

#include "types.h"

#define VERDICT_NONE     -1
#define VERDICT_INCORRECT 0
#define VERDICT_PARTIAL   1
#define VERDICT_CORRECT   2

typedef struct {
    int score;
    int verdict;
    char feedback[1024];
} AnswerVerdict;

Evaluation evaluate_answer(const char *question, const char *answer);
int evaluate_answer_score(const char *question, const char *answer, char *feedback_out, size_t feedback_len);
AnswerVerdict evaluate_answer_verdict(const char *question, const char *answer, const char *reference);
int parse_verdict_from_feedback(const char *feedback, int score);
void evaluation_init(void);
void evaluation_shutdown(void);

#endif