#include "interview.h"
#include "questions.h"
#include "question_bank.h"
#include "evaluation.h"
#include "db.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct Interview {
    char *user;
    char *domain;
    char *level;
    int questions_count;
    int current_index;
    char **questions;
    char **references;
    char **answers;
    int *question_ids;
    int *verdicts;
    int *scores;
    char **feedbacks;
    int *evaluated;
};

static void pick_question(Interview *iv, int slot) {
    int exclude[16];
    int ex_count = 0;
    for (int i = 0; i < slot && ex_count < 16; i++) {
        if (iv->question_ids[i] > 0)
            exclude[ex_count++] = iv->question_ids[i];
    }

    char text[1024], ref[1024];
    int qid = 0;
    if (questions_pick_for_interview(iv->domain, iv->level, exclude, ex_count,
            &qid, text, sizeof(text), ref, sizeof(ref), iv->user)) {
        iv->questions[slot] = strdup(text);
        iv->references[slot] = strdup(ref);
        iv->question_ids[slot] = qid;
        return;
    }

    char fallback[512];
    snprintf(fallback, sizeof(fallback),
        "Question %d (%s): Expliquez un concept fondamental de %s.",
        slot + 1, iv->level, iv->domain);
    iv->questions[slot] = strdup(fallback);
    iv->references[slot] = strdup("");
    iv->question_ids[slot] = 0;
}

Interview *interview_create(const char *user, const char *domain, const char *level, int questions_count) {
    qb_init();
    questions_ensure_loaded();
    Interview *iv = calloc(1, sizeof(Interview));
    iv->user = strdup(user ? user : "etudiant");
    iv->domain = strdup(domain ? domain : "Algorithmique");
    iv->level = strdup(level ? level : "Junior");
    iv->questions_count = questions_count > 0 ? questions_count : 5;
    int n = iv->questions_count;
    iv->questions = calloc((size_t)n, sizeof(char*));
    iv->references = calloc((size_t)n, sizeof(char*));
    iv->answers = calloc((size_t)n, sizeof(char*));
    iv->question_ids = calloc((size_t)n, sizeof(int));
    iv->verdicts = calloc((size_t)n, sizeof(int));
    iv->scores = calloc((size_t)n, sizeof(int));
    iv->feedbacks = calloc((size_t)n, sizeof(char*));
    iv->evaluated = calloc((size_t)n, sizeof(int));
    iv->current_index = -1;
    for (int i = 0; i < n; i++) {
        pick_question(iv, i);
        iv->answers[i] = NULL;
        iv->verdicts[i] = VERDICT_NONE;
        iv->scores[i] = 0;
        iv->evaluated[i] = 0;
    }
    return iv;
}

void interview_destroy(Interview *iv) {
    if (!iv) return;
    free(iv->user);
    free(iv->domain);
    free(iv->level);
    for (int i = 0; i < iv->questions_count; i++) {
        free(iv->questions[i]);
        free(iv->references[i]);
        free(iv->answers[i]);
        free(iv->feedbacks[i]);
    }
    free(iv->questions);
    free(iv->references);
    free(iv->answers);
    free(iv->question_ids);
    free(iv->verdicts);
    free(iv->scores);
    free(iv->feedbacks);
    free(iv->evaluated);
    free(iv);
}

bool interview_next(Interview *iv, char *out_question, size_t qlen) {
    if (!iv) return false;
    if (iv->current_index >= iv->questions_count - 1) return false;
    iv->current_index++;
    strncpy(out_question, iv->questions[iv->current_index], qlen - 1);
    out_question[qlen - 1] = '\0';
    return true;
}

bool interview_prev(Interview *iv, char *out_question, size_t qlen) {
    if (!iv) return false;
    if (iv->current_index <= 0) return false;
    iv->current_index--;
    strncpy(out_question, iv->questions[iv->current_index], qlen - 1);
    out_question[qlen - 1] = '\0';
    return true;
}

void interview_record_answer(Interview *iv, int question_index, const char *answer) {
    if (!iv) return;
    if (question_index < 0 || question_index >= iv->questions_count) return;
    free(iv->answers[question_index]);
    iv->answers[question_index] = strdup(answer ? answer : "");
}

int interview_evaluate_current(Interview *iv, int question_index) {
    if (!iv || question_index < 0 || question_index >= iv->questions_count) return VERDICT_NONE;
    const char *ans = iv->answers[question_index];
    if (!ans || !ans[0]) {
        iv->verdicts[question_index] = VERDICT_INCORRECT;
        iv->scores[question_index] = 0;
        free(iv->feedbacks[question_index]);
        iv->feedbacks[question_index] = strdup("FAUX — Aucune reponse fournie.");
        iv->evaluated[question_index] = 1;
        return VERDICT_INCORRECT;
    }

    AnswerVerdict av = evaluate_answer_verdict(
        iv->questions[question_index], ans, iv->references[question_index]);
    iv->verdicts[question_index] = av.verdict;
    iv->scores[question_index] = av.score;
    free(iv->feedbacks[question_index]);
    iv->feedbacks[question_index] = strdup(av.feedback);
    iv->evaluated[question_index] = 1;

    if (iv->question_ids[question_index] > 0) {
        qb_mark_answered(iv->user, iv->question_ids[question_index]);
        db_save_answer_detail(iv->user, iv->question_ids[question_index],
            iv->domain, iv->level, av.verdict, av.score, av.feedback);
    }
    return av.verdict;
}

int interview_finish(Interview *iv) {
    if (!iv) return 0;
    char categorie[256];
    snprintf(categorie, sizeof(categorie), "%s - %s", iv->domain, iv->level);
    int total = 0;
    int answered = 0;
    for (int i = 0; i < iv->questions_count; i++) {
        const char *ans = iv->answers[i];
        if (!ans || !ans[0]) continue;
        if (!iv->evaluated[i])
            interview_evaluate_current(iv, i);
        int score = iv->scores[i];
        save_score(iv->user, score, categorie);
        total += score;
        answered++;
    }
    if (answered == 0) return 0;
    return total / answered;
}

int interview_get_current_index(Interview *iv) {
    if (!iv) return -1;
    return iv->current_index;
}

bool interview_get_current_question(Interview *iv, char *out_question, size_t qlen) {
    if (!iv || iv->current_index < 0 || iv->current_index >= iv->questions_count) return false;
    strncpy(out_question, iv->questions[iv->current_index], qlen - 1);
    out_question[qlen - 1] = '\0';
    return true;
}

const char *interview_get_answer(Interview *iv, int question_index) {
    if (!iv) return NULL;
    if (question_index < 0 || question_index >= iv->questions_count) return NULL;
    return iv->answers[question_index];
}

const char *interview_get_user(Interview *iv) { return iv ? iv->user : NULL; }
const char *interview_get_domain(Interview *iv) { return iv ? iv->domain : NULL; }
const char *interview_get_level(Interview *iv) { return iv ? iv->level : NULL; }
int interview_get_questions_count(Interview *iv) { return iv ? iv->questions_count : 0; }

int interview_get_question_id(Interview *iv, int question_index) {
    if (!iv || question_index < 0 || question_index >= iv->questions_count) return 0;
    return iv->question_ids[question_index];
}

const char *interview_get_reference(Interview *iv, int question_index) {
    if (!iv || question_index < 0 || question_index >= iv->questions_count) return NULL;
    return iv->references[question_index];
}

int interview_get_verdict(Interview *iv, int question_index) {
    if (!iv || question_index < 0 || question_index >= iv->questions_count) return VERDICT_NONE;
    return iv->verdicts[question_index];
}

const char *interview_get_feedback(Interview *iv, int question_index) {
    if (!iv || question_index < 0 || question_index >= iv->questions_count) return NULL;
    return iv->feedbacks[question_index];
}

bool interview_get_question_text(Interview *iv, int question_index, char *out, size_t len) {
    if (!iv || !out || len == 0 || question_index < 0 || question_index >= iv->questions_count) return false;
    strncpy(out, iv->questions[question_index] ? iv->questions[question_index] : "", len - 1);
    out[len - 1] = '\0';
    return out[0] != '\0';
}

int interview_get_score(Interview *iv, int question_index) {
    if (!iv || question_index < 0 || question_index >= iv->questions_count) return 0;
    return iv->scores[question_index];
}

int interview_compute_average(Interview *iv) {
    if (!iv) return 0;
    int total = 0, n = 0;
    for (int i = 0; i < iv->questions_count; i++) {
        const char *ans = iv->answers[i];
        if (!ans || !ans[0]) continue;
        if (!iv->evaluated[i]) interview_evaluate_current(iv, i);
        total += iv->scores[i];
        n++;
    }
    return n > 0 ? total / n : 0;
}