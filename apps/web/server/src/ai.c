#include "ai.h"

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"

typedef struct {
    char *data;
    size_t size;
} Memory;

static size_t write_cb(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    Memory *mem = (Memory *)userp;
    char *ptr = realloc(mem->data, mem->size + realsize + 1);

    if (!ptr) {
        return 0;
    }

    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = '\0';
    return realsize;
}

int backend_ai_prompt(const char *prompt, char **out_reply, char *err, size_t errlen) {
    CURL *curl = NULL;
    CURLcode res;
    struct curl_slist *headers = NULL;
    Memory response;
    cJSON *root = NULL;
    cJSON *messages = NULL;
    cJSON *sys_msg = NULL;
    cJSON *user_msg = NULL;
    cJSON *parsed = NULL;
    cJSON *choices = NULL;
    cJSON *choice0 = NULL;
    cJSON *message = NULL;
    cJSON *content = NULL;
    char *post_data = NULL;
    const char *api_key = getenv("AURA_API_KEY");
    char auth_header[512];

    if (!prompt || !out_reply) {
        if (err) snprintf(err, errlen, "missing prompt");
        return 0;
    }

    if (!api_key || api_key[0] == '\0') {
        if (err) snprintf(err, errlen, "AURA_API_KEY is not configured");
        return 0;
    }

    response.data = malloc(1);
    response.size = 0;
    if (!response.data) {
        if (err) snprintf(err, errlen, "out of memory");
        return 0;
    }
    response.data[0] = '\0';

    root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "model", "llama-3.1-8b-instant");
    messages = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "messages", messages);

    sys_msg = cJSON_CreateObject();
    cJSON_AddStringToObject(sys_msg, "role", "system");
    cJSON_AddStringToObject(sys_msg, "content", "You are a concise assistant. Reply in plain text.");
    cJSON_AddItemToArray(messages, sys_msg);

    user_msg = cJSON_CreateObject();
    cJSON_AddStringToObject(user_msg, "role", "user");
    cJSON_AddStringToObject(user_msg, "content", prompt);
    cJSON_AddItemToArray(messages, user_msg);

    post_data = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (!curl) {
        free(post_data);
        free(response.data);
        if (err) snprintf(err, errlen, "curl init failed");
        return 0;
    }

    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", api_key);

    headers = curl_slist_append(headers, auth_header);
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.groq.com/openai/v1/chat/completions");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

    res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    free(post_data);

    if (res != CURLE_OK) {
        if (err) snprintf(err, errlen, "AI request failed: %s", curl_easy_strerror(res));
        free(response.data);
        return 0;
    }

    parsed = cJSON_Parse(response.data);
    free(response.data);

    if (!parsed) {
        if (err) snprintf(err, errlen, "AI response is not valid JSON");
        return 0;
    }

    choices = cJSON_GetObjectItemCaseSensitive(parsed, "choices");
    choice0 = cJSON_GetArrayItem(choices, 0);
    message = cJSON_GetObjectItemCaseSensitive(choice0, "message");
    content = cJSON_GetObjectItemCaseSensitive(message, "content");

    if (!cJSON_IsString(content) || !content->valuestring) {
        cJSON_Delete(parsed);
        if (err) snprintf(err, errlen, "AI response missing content");
        return 0;
    }

    *out_reply = strdup(content->valuestring);
    cJSON_Delete(parsed);

    if (!*out_reply) {
        if (err) snprintf(err, errlen, "out of memory");
        return 0;
    }

    return 1;
}
