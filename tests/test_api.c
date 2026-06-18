#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/api.h"
#include "../include/evaluation.h"

static int tests_run = 0;
static int tests_failed = 0;

static void assert_eq_int(const char *name, int expected, int actual) {
    tests_run++;
    if (expected != actual) {
        tests_failed++;
        fprintf(stderr, "FAIL %s: expected %d, got %d\n", name, expected, actual);
    } else {
        printf("OK   %s\n", name);
    }
}

static void assert_nonnull(const char *name, const void *ptr) {
    tests_run++;
    if (ptr == NULL) {
        tests_failed++;
        fprintf(stderr, "FAIL %s: expected non-null pointer\n", name);
    } else {
        printf("OK   %s\n", name);
    }
}

static void assert_contains(const char *name, const char *haystack, const char *needle) {
    tests_run++;
    if (!haystack || !needle || strstr(haystack, needle) == NULL) {
        tests_failed++;
        fprintf(stderr, "FAIL %s: '%s' not found in '%s'\n", name, needle, haystack ? haystack : "(null)");
    } else {
        printf("OK   %s\n", name);
    }
}

int main(void) {
    assert_eq_int("parse_score_note_8", 8, parse_score_from_feedback("Note: 8/10 — bonne reponse"));
    assert_eq_int("parse_score_note_10", 10, parse_score_from_feedback("Score final 10 sur 10"));
    assert_eq_int("parse_score_empty", 0, parse_score_from_feedback(""));

    const char *mock_json =
        "{\"choices\":[{\"message\":{\"content\":\"Bonne reponse, note 7/10.\"}}]}";
    char *parsed = parse_ai_response(mock_json);
    assert_nonnull("parse_ai_response_content", parsed);
    if (parsed) {
        assert_contains("parse_ai_response_text", parsed, "7/10");
        free(parsed);
    }

    const char *bad_json = "{\"error\":\"invalid\"}";
    char *bad = parse_ai_response(bad_json);
    tests_run++;
    if (bad != NULL) {
        tests_failed++;
        fprintf(stderr, "FAIL parse_ai_response_invalid: expected NULL\n");
        free(bad);
    } else {
        printf("OK   parse_ai_response_invalid\n");
    }

    assert_eq_int("verdict_correct", VERDICT_CORRECT,
        parse_verdict_from_feedback("VERDICT: CORRECT\nSCORE: 9/10", 9));
    assert_eq_int("verdict_incorrect", VERDICT_INCORRECT,
        parse_verdict_from_feedback("VERDICT: INCORRECT\nSCORE: 2/10", 2));
    assert_eq_int("verdict_partial", VERDICT_PARTIAL,
        parse_verdict_from_feedback("VERDICT: PARTIEL\nSCORE: 6/10", 6));

    printf("\n%d tests, %d failures\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}