#ifndef API_H
#define API_H

char* demander_question(int categorie);
char* soumettre_reponse(const char* question_posee, const char* reponse_utilisateur);
char* soumettre_reponse_avec_reference(const char* question_posee, const char* reponse_utilisateur, const char* reference);
char* parse_ai_response(const char* json_string);
int parse_score_from_feedback(const char *feedback);
int parse_verdict_from_feedback(const char *feedback, int score);
char* generer_question_ia(const char *domain, const char *level, const char *topic_hint);

#endif // API_H