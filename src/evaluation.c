#include "evaluation.h"
#include "api.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>

static int str_contains_ci(const char *hay, const char *needle) {
    if (!hay || !needle || !needle[0]) return 0;
    size_t nlen = strlen(needle);
    for (const char *p = hay; *p; p++) {
        size_t i = 0;
        while (i < nlen && p[i] && tolower((unsigned char)p[i]) == tolower((unsigned char)needle[i])) i++;
        if (i == nlen) return 1;
    }
    return 0;
}

static int local_verdict_from_reference(const char *answer, const char *reference) {
    if (!answer || !answer[0]) return VERDICT_INCORRECT;
    if (!reference || !reference[0]) {
        size_t len = strlen(answer);
        if (len >= 80) return VERDICT_CORRECT;
        if (len >= 25) return VERDICT_PARTIAL;
        return VERDICT_INCORRECT;
    }
    char ref_copy[512];
    strncpy(ref_copy, reference, sizeof(ref_copy) - 1);
    ref_copy[sizeof(ref_copy) - 1] = '\0';
    int tokens = 0, hits = 0;
    char *tok = strtok(ref_copy, " ,;.");
    while (tok) {
        if (strlen(tok) >= 4) {
            tokens++;
            if (str_contains_ci(answer, tok)) hits++;
        }
        tok = strtok(NULL, " ,;.");
    }
    if (tokens == 0) {
        size_t len = strlen(answer);
        if (len >= 60) return VERDICT_CORRECT;
        if (len >= 20) return VERDICT_PARTIAL;
        return VERDICT_INCORRECT;
    }
    float ratio = (float)hits / (float)tokens;
    if (ratio >= 0.45f) return VERDICT_CORRECT;
    if (ratio >= 0.2f || strlen(answer) >= 40) return VERDICT_PARTIAL;
    return VERDICT_INCORRECT;
}

static int verdict_to_score(int verdict) {
    switch (verdict) {
        case VERDICT_CORRECT: return 9;
        case VERDICT_PARTIAL: return 6;
        case VERDICT_INCORRECT: return 2;
        default: return 0;
    }
}

Evaluation evaluate_answer(const char *question, const char *answer) {
    Evaluation e;
    AnswerVerdict av = evaluate_answer_verdict(question, answer, NULL);
    e.score = (float)av.score * 10.0f / 9.0f;
    e.clarity = av.score;
    e.confidence = av.score;
    e.communication = av.score;
    (void)question;
    return e;
}

int evaluate_answer_score(const char *question, const char *answer, char *feedback_out, size_t feedback_len) {
    AnswerVerdict av = evaluate_answer_verdict(question, answer, NULL);
    if (feedback_out && feedback_len > 0)
        snprintf(feedback_out, feedback_len, "%s", av.feedback);
    return av.score;
}

AnswerVerdict evaluate_answer_verdict(const char *question, const char *answer, const char *reference) {
    AnswerVerdict av;
    memset(&av, 0, sizeof(av));
    av.verdict = VERDICT_NONE;

    if (!question || !answer || !answer[0]) {
        av.verdict = VERDICT_INCORRECT;
        av.score = 0;
        snprintf(av.feedback, sizeof(av.feedback), "FAUX — Reponse vide ou absente.");
        return av;
    }

    char *raw = soumettre_reponse_avec_reference(question, answer, reference);
    if (raw) {
        char *text = parse_ai_response(raw);
        free(raw);
        if (text && text[0]) {
            av.score = parse_score_from_feedback(text);
            av.verdict = parse_verdict_from_feedback(text, av.score);
            if (av.verdict == VERDICT_NONE)
                av.verdict = local_verdict_from_reference(answer, reference);
            if (av.verdict == VERDICT_CORRECT)
                snprintf(av.feedback, sizeof(av.feedback), "VRAI — %s", text);
            else if (av.verdict == VERDICT_PARTIAL)
                snprintf(av.feedback, sizeof(av.feedback), "PARTIEL — %s", text);
            else
                snprintf(av.feedback, sizeof(av.feedback), "FAUX — %s", text);
            free(text);
            return av;
        }
        if (text) free(text);
    }

    av.verdict = local_verdict_from_reference(answer, reference);
    av.score = verdict_to_score(av.verdict);
    if (av.verdict == VERDICT_CORRECT)
        snprintf(av.feedback, sizeof(av.feedback),
            "VRAI — Evaluation locale (IA indisponible). Bonne couverture des concepts.");
    else if (av.verdict == VERDICT_PARTIAL)
        snprintf(av.feedback, sizeof(av.feedback),
            "PARTIEL — Evaluation locale: reponse incomplete, developpez davantage.");
    else
        snprintf(av.feedback, sizeof(av.feedback),
            "FAUX — Evaluation locale: reponse trop courte ou hors sujet.");
    return av;
}

void evaluation_init(void) {}
void evaluation_shutdown(void) {}