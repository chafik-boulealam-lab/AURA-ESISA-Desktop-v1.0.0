#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <stddef.h>

typedef struct Question {
    char *id;
    char *domain;
    char *level;
    char *text;
    char **choices;
    int choices_count;
    char *answer;
} Question;

typedef struct History {
    char *username;
    char *domain;
    char *level;
    char *date;
    float score;
    int questions_count;
} History;

typedef struct Statistics {
    int interviews_taken;
    float average_score;
} Statistics;

typedef struct Evaluation {
    float score;
    int clarity;
    int confidence;
    int communication;
} Evaluation;

#endif // TYPES_H
