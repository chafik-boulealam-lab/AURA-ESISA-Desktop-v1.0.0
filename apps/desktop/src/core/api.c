#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "api.h"
#include "evaluation.h"
#include "config.h"
#include "filesystem.h"
#include "cJSON.h"

// Callback pour l'animation d'attente (Spinner ASCII)
static int progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    // Ignorer les avertissements "paramètres non utilisés"
    (void)clientp; (void)dltotal; (void)dlnow; (void)ultotal; (void)ulnow;
    
    static int i = 0;
    const char spinner[] = "|/-\\";
    
    // '\r' renvoie le curseur au début de la ligne pour écraser le texte précédent
    printf("\r\x1b[36m> En attente de l'IA (Groq)... %c \x1b[0m", spinner[i++ % 4]);
    fflush(stdout); // Force l'affichage immédiatement
    
    return 0; // 0 = continuer le téléchargement
}

// Struct pour stocker la réponse de curl en mémoire
struct MemoryStruct {
    char *memory;
    size_t size;
};

// =====================================================================
// EXPLICATION SOUTENANCE : WriteMemoryCallback
// =====================================================================
// Cette fonction est un "callback" (une fonction de rappel) utilisée par libcurl.
// Lorsque libcurl télécharge les données de la réponse HTTP sur le réseau, 
// il ne les reçoit pas toutes d'un coup, mais par "morceaux" (chunks).
// 
// À chaque morceau reçu, libcurl appelle cette fonction.
// 
// Pointers expliqués :
// - `void *contents` : Un pointeur générique "void*" qui pointe vers les données brutes 
//   arrivées depuis le réseau.
// - `void *userp` : Un pointeur qui pointe vers notre structure de données personnalisée 
//   (`struct MemoryStruct *mem`), où l'on va accumuler la réponse.
// 
// Mémoire dynamique : 
// On utilise `realloc()` pour agrandir la taille du bloc mémoire cible afin d'y ajouter
// le nouveau texte. Ensuite, `memcpy()` copie exactement la réponse du réseau (contents) 
// à la fin du bloc mémoire agrandi (mem->memory).
// =====================================================================
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if(ptr == NULL) {
        printf("pas assez de memoire (realloc a echoue)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

// =====================================================================
// EXPLICATION SOUTENANCE : ask_ai() et libcurl
// =====================================================================
// Cette fonction centralise la configuration et l'envoi de la requête réseau.
// 
// Gestion du réseau par libcurl :
// 1. curl_easy_init() : Initialise un "handle" de session réseau (pointeur CURL *).
// 2. curl_slist : C'est une liste chaînée (utilisant des pointeurs) gérée par curl 
//    pour ajouter nos en-têtes HTTP (L'autorisation API et le Content-Type Json).
// 3. curl_easy_setopt() : Permet de configurer l'URL cible, les headers, le payload JSON,
//    et très important, de lier notre pointeur de données (`&chunk`) avec 
//    la fonction callback de lecture qu'on a défini (`WriteMemoryCallback`).
// 4. curl_easy_perform() : Bloque l'exécution temporelle et exécute la connexion 
//    avec les serveurs distants. C'est ici que l'échange réseau TCP/HTTP a lieu.
// 
// Mémoire : L'espace `chunk.memory` dynamiquement alloué doit impérativement 
// être libéré (`free()`) par l'appelant après récupération du résultat pour éviter 
// une "fuite de mémoire" (Memory leak).
// =====================================================================
static char* send_curl_request(const char* post_data) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;
    
    chunk.memory = malloc(1);
    chunk.size = 0;

    const char* api_key = getenv("AURA_API_KEY");
    if (api_key == NULL || api_key[0] == '\0') {
        api_key = aura_config_get_string("groq_api_key", "");
    }
    if (api_key == NULL || api_key[0] == '\0') {
        fprintf(stderr, "Erreur : AURA_API_KEY non configuree (variable d'environnement ou groq_api_key dans config/aura.cfg).\n");
        free(chunk.memory);
        return NULL;
    }

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    
    if(curl) {
        struct curl_slist *headers = NULL;
        
        char auth_header[256];
        snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", api_key);
        headers = curl_slist_append(headers, auth_header);
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, "https://api.groq.com/openai/v1/chat/completions");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
        
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        {
            const char *root = aura_fs_get_app_root();
            char cainfo[1200];
#ifdef _WIN32
            snprintf(cainfo, sizeof(cainfo), "%s\\cacert.pem", root ? root : ".");
#else
            snprintf(cainfo, sizeof(cainfo), "%s/cacert.pem", root ? root : ".");
#endif
            if (aura_fs_file_exists(cainfo)) {
                curl_easy_setopt(curl, CURLOPT_CAINFO, cainfo);
            } else {
#ifdef _WIN32
                snprintf(cainfo, sizeof(cainfo), "%s\\ca-bundle.crt", root ? root : ".");
#else
                snprintf(cainfo, sizeof(cainfo), "%s/ca-bundle.crt", root ? root : ".");
#endif
                if (aura_fs_file_exists(cainfo))
                    curl_easy_setopt(curl, CURLOPT_CAINFO, cainfo);
            }
        }

        res = curl_easy_perform(curl);
        
        if(res != CURLE_OK) {
            fprintf(stderr, "Erreur de connexion : %s\n", curl_easy_strerror(res));
            free(chunk.memory);
            chunk.memory = NULL;
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    
    curl_global_cleanup();
    
    return chunk.memory;
}

char* demander_question(int categorie) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "model", "llama-3.1-8b-instant");
    
    cJSON *messages = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "messages", messages);
    
    cJSON *sys_msg = cJSON_CreateObject();
    cJSON_AddStringToObject(sys_msg, "role", "system");
    
    const char *sys_prompt = "Tu es un assistant IA.";
    switch(categorie) {
        case 1:
            sys_prompt = "Tu es un examinateur ESISA en Architecture des ordinateurs. Pose UNE question d'entretien technique (CPU, memoire, bus, pipeline). Reponds uniquement avec la question.";
            break;
        case 2:
            sys_prompt = "Tu es un examinateur ESISA en Algorithmique. Pose UNE question d'entretien (complexite, structures, graphes, tri). Reponds uniquement avec la question.";
            break;
        case 3:
            sys_prompt = "Tu es un examinateur ESISA en Programmation C. Pose UNE question d'entretien (pointeurs, malloc, structs, norme C). Reponds uniquement avec la question.";
            break;
    }
    cJSON_AddStringToObject(sys_msg, "content", sys_prompt);
    cJSON_AddItemToArray(messages, sys_msg);

    cJSON *user_msg = cJSON_CreateObject();
    cJSON_AddStringToObject(user_msg, "role", "user");
    cJSON_AddStringToObject(user_msg, "content", "Génère la question.");
    cJSON_AddItemToArray(messages, user_msg);

    char *post_data = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    char *response = send_curl_request(post_data);
    free(post_data);
    
    return response;
}

char* soumettre_reponse(const char* question_posee, const char* reponse_utilisateur) {
    return soumettre_reponse_avec_reference(question_posee, reponse_utilisateur, NULL);
}

char* soumettre_reponse_avec_reference(const char* question_posee, const char* reponse_utilisateur, const char* reference) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "model", "llama-3.1-8b-instant");

    cJSON *messages = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "messages", messages);

    cJSON *sys_msg = cJSON_CreateObject();
    cJSON_AddStringToObject(sys_msg, "role", "system");
    cJSON_AddStringToObject(sys_msg, "content",
        "Tu corriges des reponses d'entretien technique ESISA. "
        "Reponds en francais, concis. "
        "Format OBLIGATOIRE en fin de message:\n"
        "VERDICT: CORRECT ou PARTIEL ou INCORRECT\n"
        "SCORE: X/10\n"
        "Puis 2-3 phrases de feedback.");
    cJSON_AddItemToArray(messages, sys_msg);

    cJSON *user_msg = cJSON_CreateObject();
    cJSON_AddStringToObject(user_msg, "role", "user");

    char buffer[4096];
    if (reference && reference[0]) {
        snprintf(buffer, sizeof(buffer),
            "Question: %s\nReponse etudiant: %s\nReponse de reference: %s\n"
            "Evalue si la reponse est correcte, partielle ou fausse.",
            question_posee, reponse_utilisateur, reference);
    } else {
        snprintf(buffer, sizeof(buffer),
            "Question: %s\nReponse etudiant: %s\nEvalue la reponse.",
            question_posee, reponse_utilisateur);
    }

    cJSON_AddStringToObject(user_msg, "content", buffer);
    cJSON_AddItemToArray(messages, user_msg);

    char *post_data = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    char *response = send_curl_request(post_data);
    free(post_data);

    return response;
}

char* generer_question_ia(const char *domain, const char *level, const char *topic_hint) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "model", "llama-3.1-8b-instant");

    cJSON *messages = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "messages", messages);

    cJSON *sys_msg = cJSON_CreateObject();
    cJSON_AddStringToObject(sys_msg, "role", "system");
    cJSON_AddStringToObject(sys_msg, "content",
        "Tu generes UNE question d'entretien technique unique pour ESISA. "
        "Reponds uniquement avec la question, sans prefixe.");
    cJSON_AddItemToArray(messages, sys_msg);

    cJSON *user_msg = cJSON_CreateObject();
    cJSON_AddStringToObject(user_msg, "role", "user");
    char buffer[1024];
    snprintf(buffer, sizeof(buffer),
        "Matiere: %s. Niveau: %s. Sujet: %s. Question originale, non repetitive.",
        domain ? domain : "Algorithmique",
        level ? level : "Junior",
        topic_hint ? topic_hint : "general");
    cJSON_AddStringToObject(user_msg, "content", buffer);
    cJSON_AddItemToArray(messages, user_msg);

    char *post_data = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    char *response = send_curl_request(post_data);
    free(post_data);
    return response;
}

// =====================================================================
// EXPLICATION SOUTENANCE : parse_ai_response() et cJSON
// =====================================================================
// Cette fonction lit une chaîne de texte brute au format JSON et la transforme 
// en une arborescence d'objets manipulable via la librairie cJSON.
// 
// Pointers expliqués :
// - `const char* json_string` : Pointeur constant vers le texte brut de la réponse réseau.
// - `cJSON *json` : Un pointeur vers la racine de l'arbre représentant les hiérarchies
//   du fichier JSON en mémoire dynamique.
// 
// Navigation : On traverse cet arbre (json -> choices -> [0] -> message -> content)
// en chaînant les appels. Chaque étape retourne un pointeur vers le noeud enfant.
// Une fois le texte final trouvé, on alloue (`malloc`) un nouveau pointeur texte (`result`) 
// pour ne copier que la réponse pertinente.
//
// Mémoire : Il est impératif d'utiliser `cJSON_Delete(json)` pour détruire la racine  
// qui effacera de manière récursive tous les sous-noeuds alloués par la validation JSON !
// =====================================================================
int parse_score_from_feedback(const char *feedback) {
    if (!feedback || !feedback[0]) return 0;
    const char *score_tag = strstr(feedback, "SCORE:");
    if (score_tag) {
        int val = parse_score_from_feedback(score_tag + 6);
        if (val > 0) return val;
    }
    const char *p = feedback;
    while (*p) {
        if ((*p >= '0' && *p <= '9') && (p == feedback || *(p-1) == ' ' || *(p-1) == '/' || *(p-1) == ':')) {
            int val = 0;
            while (*p >= '0' && *p <= '9') { val = val * 10 + (*p - '0'); p++; }
            if (val >= 0 && val <= 10) return val;
        }
        p++;
    }
    return 5;
}

int parse_verdict_from_feedback(const char *feedback, int score) {
    if (!feedback || !feedback[0]) {
        if (score >= 8) return VERDICT_CORRECT;
        if (score >= 5) return VERDICT_PARTIAL;
        return VERDICT_INCORRECT;
    }
    if (strstr(feedback, "VERDICT: CORRECT") || strstr(feedback, "VERDICT:CORRECT"))
        return VERDICT_CORRECT;
    if (strstr(feedback, "VERDICT: PARTIEL") || strstr(feedback, "VERDICT:PARTIEL")
        || strstr(feedback, "VERDICT: PARTIAL") || strstr(feedback, "VERDICT:PARTIAL"))
        return VERDICT_PARTIAL;
    if (strstr(feedback, "VERDICT: INCORRECT") || strstr(feedback, "VERDICT:INCORRECT")
        || strstr(feedback, "VERDICT: FAUX") || strstr(feedback, "VERDICT:FAUX"))
        return VERDICT_INCORRECT;
    if (strstr(feedback, "VRAI") && !strstr(feedback, "FAUX")) return VERDICT_CORRECT;
    if (strstr(feedback, "FAUX")) return VERDICT_INCORRECT;
    if (strstr(feedback, "PARTIEL")) return VERDICT_PARTIAL;
    if (score >= 8) return VERDICT_CORRECT;
    if (score >= 5) return VERDICT_PARTIAL;
    return VERDICT_INCORRECT;
}

char* parse_ai_response(const char* json_string) {
    if (json_string == NULL) return NULL;

    cJSON *json = cJSON_Parse(json_string);
    if (json == NULL) {
        fprintf(stderr, "Erreur de parsing JSON.\n");
        return NULL;
    }

    // Navigation dans le JSON : choices[0]->message->content
    cJSON *choices = cJSON_GetObjectItemCaseSensitive(json, "choices");
    cJSON *choice = cJSON_GetArrayItem(choices, 0);
    cJSON *message = cJSON_GetObjectItemCaseSensitive(choice, "message");
    cJSON *content = cJSON_GetObjectItemCaseSensitive(message, "content");

    char *result = NULL;
    
    // Si on a bien trouvé du texte, on le copie localement
    if (cJSON_IsString(content) && (content->valuestring != NULL)) {
        result = malloc(strlen(content->valuestring) + 1);
        if (result) {
            strcpy(result, content->valuestring);
        }
    } else {
        fprintf(stderr, "Impossible de trouver 'content' dans la réponse !.\n");
    }

    // Libérer proprement l'objet JSON (EVITER LES FUITES DE MEMOIRE)
    cJSON_Delete(json);
    
    return result;
}