#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"

#include "ai.h"
#include "db.h"

#define SERVER_PORT 8080
#define REQ_BUF_SIZE 65536

static int ci_equal(char a, char b) {
    if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
    if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
    return a == b;
}

static int str_ieq(const char *a, const char *b) {
    size_t i = 0;
    if (!a || !b) return 0;
    while (a[i] && b[i]) {
        if (!ci_equal(a[i], b[i])) return 0;
        i++;
    }
    return a[i] == '\0' && b[i] == '\0';
}

static int starts_with(const char *s, const char *prefix) {
    size_t i;
    if (!s || !prefix) return 0;
    for (i = 0; prefix[i]; i++) {
        if (s[i] != prefix[i]) return 0;
    }
    return 1;
}

static const char *http_status_text(int status) {
    switch (status) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 500: return "Internal Server Error";
        default: return "OK";
    }
}

static int send_all(SOCKET client, const char *data, int len) {
    int sent_total = 0;
    while (sent_total < len) {
        int sent = send(client, data + sent_total, len - sent_total, 0);
        if (sent <= 0) return 0;
        sent_total += sent;
    }
    return 1;
}

static void send_json(SOCKET client, int status, const char *json_body) {
    char header[512];
    int body_len = (int)strlen(json_body);
    int n = snprintf(
        header,
        sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: application/json\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n\r\n",
        status,
        http_status_text(status),
        body_len
    );

    send_all(client, header, n);
    send_all(client, json_body, body_len);
}

static void send_error(SOCKET client, int status, const char *message) {
    char body[512];
    snprintf(body, sizeof(body), "{\"ok\":false,\"error\":\"%s\"}", message ? message : "unknown error");
    send_json(client, status, body);
}

static int parse_content_length(const char *headers) {
    const char *p = strstr(headers, "Content-Length:");
    if (!p) return 0;
    p += strlen("Content-Length:");
    while (*p == ' ') p++;
    return atoi(p);
}

static char *find_header_end(char *buf) {
    return strstr(buf, "\r\n\r\n");
}

static int read_http_request(SOCKET client, char *buf, size_t cap, size_t *out_len) {
    size_t total = 0;
    int content_len = 0;
    char *header_end = NULL;

    while (total < cap - 1) {
        int r = recv(client, buf + total, (int)(cap - total - 1), 0);
        if (r <= 0) return 0;
        total += (size_t)r;
        buf[total] = '\0';

        if (!header_end) {
            header_end = find_header_end(buf);
            if (header_end) {
                size_t header_len = (size_t)(header_end - buf) + 4;
                content_len = parse_content_length(buf);
                if (total >= header_len + (size_t)content_len) {
                    *out_len = total;
                    return 1;
                }
            }
        } else {
            size_t header_len = (size_t)(header_end - buf) + 4;
            if (total >= header_len + (size_t)content_len) {
                *out_len = total;
                return 1;
            }
        }
    }

    return 0;
}

static int get_query_param(const char *path, const char *key, char *out, size_t outlen) {
    const char *q = strchr(path, '?');
    size_t key_len = strlen(key);
    if (!q) return 0;
    q++;

    while (*q) {
        const char *eq = strchr(q, '=');
        const char *amp;
        if (!eq) break;
        amp = strchr(eq + 1, '&');
        if (!amp) amp = q + strlen(q);

        if ((size_t)(eq - q) == key_len && strncmp(q, key, key_len) == 0) {
            size_t len = (size_t)(amp - (eq + 1));
            if (len >= outlen) len = outlen - 1;
            memcpy(out, eq + 1, len);
            out[len] = '\0';
            return 1;
        }

        if (*amp == '&') q = amp + 1;
        else break;
    }

    return 0;
}

static int path_matches(const char *path, const char *api_path) {
    char clean_path[512];
    const char *q;
    size_t len;

    if (!path || !api_path) return 0;

    q = strchr(path, '?');
    if (q) len = (size_t)(q - path);
    else len = strlen(path);

    if (len >= sizeof(clean_path)) len = sizeof(clean_path) - 1;
    memcpy(clean_path, path, len);
    clean_path[len] = '\0';

    if (str_ieq(clean_path, api_path)) return 1;
    if (starts_with(api_path, "/api/")) {
        const char *short_path = api_path + 4;
        if (str_ieq(clean_path, short_path)) return 1;
    }
    return 0;
}

static cJSON *parse_body_json(const char *body) {
    if (!body || body[0] == '\0') return NULL;
    return cJSON_Parse(body);
}

static void handle_register(SOCKET client, const char *body) {
    cJSON *json = parse_body_json(body);
    cJSON *email;
    cJSON *password;
    char err[256];

    if (!json) {
        send_error(client, 400, "invalid JSON");
        return;
    }

    email = cJSON_GetObjectItemCaseSensitive(json, "email");
    password = cJSON_GetObjectItemCaseSensitive(json, "password");

    if (!cJSON_IsString(email) || !cJSON_IsString(password)) {
        cJSON_Delete(json);
        send_error(client, 400, "email and password are required");
        return;
    }

    if (!backend_register_user(email->valuestring, password->valuestring, err, sizeof(err))) {
        cJSON_Delete(json);
        send_error(client, 400, err);
        return;
    }

    cJSON_Delete(json);
    send_json(client, 201, "{\"ok\":true,\"message\":\"registered\"}");
}

static void handle_login(SOCKET client, const char *body) {
    cJSON *json = parse_body_json(body);
    cJSON *email;
    cJSON *password;
    char err[256];

    if (!json) {
        send_error(client, 400, "invalid JSON");
        return;
    }

    email = cJSON_GetObjectItemCaseSensitive(json, "email");
    password = cJSON_GetObjectItemCaseSensitive(json, "password");

    if (!cJSON_IsString(email) || !cJSON_IsString(password)) {
        cJSON_Delete(json);
        send_error(client, 400, "email and password are required");
        return;
    }

    if (!backend_login_user(email->valuestring, password->valuestring, err, sizeof(err))) {
        cJSON_Delete(json);
        send_error(client, 400, err);
        return;
    }

    cJSON_Delete(json);
    send_json(client, 200, "{\"ok\":true,\"message\":\"login success\"}");
}

static void handle_tasks_get(SOCKET client, const char *path) {
    char email[256];
    char err[256];
    char *tasks_json;
    char *resp;
    size_t len;

    if (!get_query_param(path, "email", email, sizeof(email))) {
        send_error(client, 400, "email query parameter is required");
        return;
    }

    tasks_json = backend_list_tasks_json(email, err, sizeof(err));
    if (!tasks_json) {
        send_error(client, 500, err);
        return;
    }

    len = strlen(tasks_json) + 64;
    resp = (char *)malloc(len);
    if (!resp) {
        free(tasks_json);
        send_error(client, 500, "out of memory");
        return;
    }

    snprintf(resp, len, "{\"ok\":true,\"tasks\":%s}", tasks_json);
    send_json(client, 200, resp);

    free(tasks_json);
    free(resp);
}

static void handle_tasks_post(SOCKET client, const char *body) {
    cJSON *json = parse_body_json(body);
    cJSON *email;
    cJSON *title;
    char err[256];

    if (!json) {
        send_error(client, 400, "invalid JSON");
        return;
    }

    email = cJSON_GetObjectItemCaseSensitive(json, "email");
    title = cJSON_GetObjectItemCaseSensitive(json, "title");

    if (!cJSON_IsString(email) || !cJSON_IsString(title)) {
        cJSON_Delete(json);
        send_error(client, 400, "email and title are required");
        return;
    }

    if (!backend_add_task(email->valuestring, title->valuestring, err, sizeof(err))) {
        cJSON_Delete(json);
        send_error(client, 400, err);
        return;
    }

    cJSON_Delete(json);
    send_json(client, 201, "{\"ok\":true,\"message\":\"task added\"}");
}

static void handle_notes_get(SOCKET client, const char *path) {
    char email[256];
    char err[256];
    char *notes_json;
    char *resp;
    size_t len;

    if (!get_query_param(path, "email", email, sizeof(email))) {
        send_error(client, 400, "email query parameter is required");
        return;
    }

    notes_json = backend_list_notes_json(email, err, sizeof(err));
    if (!notes_json) {
        send_error(client, 500, err);
        return;
    }

    len = strlen(notes_json) + 64;
    resp = (char *)malloc(len);
    if (!resp) {
        free(notes_json);
        send_error(client, 500, "out of memory");
        return;
    }

    snprintf(resp, len, "{\"ok\":true,\"notes\":%s}", notes_json);
    send_json(client, 200, resp);

    free(notes_json);
    free(resp);
}

static void handle_notes_post(SOCKET client, const char *body) {
    cJSON *json = parse_body_json(body);
    cJSON *email;
    cJSON *content;
    char err[256];

    if (!json) {
        send_error(client, 400, "invalid JSON");
        return;
    }

    email = cJSON_GetObjectItemCaseSensitive(json, "email");
    content = cJSON_GetObjectItemCaseSensitive(json, "content");

    if (!cJSON_IsString(email) || !cJSON_IsString(content)) {
        cJSON_Delete(json);
        send_error(client, 400, "email and content are required");
        return;
    }

    if (!backend_add_note(email->valuestring, content->valuestring, err, sizeof(err))) {
        cJSON_Delete(json);
        send_error(client, 400, err);
        return;
    }

    cJSON_Delete(json);
    send_json(client, 201, "{\"ok\":true,\"message\":\"note added\"}");
}

static void handle_ai_post(SOCKET client, const char *body) {
    cJSON *json = parse_body_json(body);
    cJSON *prompt;
    char err[512];
    char *reply = NULL;
    cJSON *resp = NULL;
    char *resp_txt = NULL;

    if (!json) {
        send_error(client, 400, "invalid JSON");
        return;
    }

    prompt = cJSON_GetObjectItemCaseSensitive(json, "prompt");
    if (!cJSON_IsString(prompt)) {
        cJSON_Delete(json);
        send_error(client, 400, "prompt is required");
        return;
    }

    if (!backend_ai_prompt(prompt->valuestring, &reply, err, sizeof(err))) {
        cJSON_Delete(json);
        send_error(client, 500, err);
        return;
    }

    resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "ok", 1);
    cJSON_AddStringToObject(resp, "reply", reply);
    resp_txt = cJSON_PrintUnformatted(resp);

    send_json(client, 200, resp_txt);

    free(reply);
    free(resp_txt);
    cJSON_Delete(resp);
    cJSON_Delete(json);
}

static void route_request(SOCKET client, const char *method, const char *path, const char *body) {
    if (str_ieq(method, "OPTIONS")) {
        send_json(client, 204, "{}");
        return;
    }

    if (path_matches(path, "/api/register")) {
        if (!str_ieq(method, "POST")) {
            send_error(client, 405, "method not allowed");
            return;
        }
        handle_register(client, body);
        return;
    }

    if (path_matches(path, "/api/login")) {
        if (!str_ieq(method, "POST")) {
            send_error(client, 405, "method not allowed");
            return;
        }
        handle_login(client, body);
        return;
    }

    if (path_matches(path, "/api/tasks")) {
        if (str_ieq(method, "GET")) {
            handle_tasks_get(client, path);
            return;
        }
        if (str_ieq(method, "POST")) {
            handle_tasks_post(client, body);
            return;
        }
        send_error(client, 405, "method not allowed");
        return;
    }

    if (path_matches(path, "/api/notes")) {
        if (str_ieq(method, "GET")) {
            handle_notes_get(client, path);
            return;
        }
        if (str_ieq(method, "POST")) {
            handle_notes_post(client, body);
            return;
        }
        send_error(client, 405, "method not allowed");
        return;
    }

    if (path_matches(path, "/api/ai")) {
        if (!str_ieq(method, "POST")) {
            send_error(client, 405, "method not allowed");
            return;
        }
        handle_ai_post(client, body);
        return;
    }

    send_error(client, 404, "route not found");
}

static void handle_client(SOCKET client) {
    char req[REQ_BUF_SIZE];
    size_t req_len = 0;
    char method[16] = {0};
    char path[512] = {0};
    char *header_end;
    const char *body = "";

    if (!read_http_request(client, req, sizeof(req), &req_len)) {
        send_error(client, 400, "invalid request");
        return;
    }

    if (sscanf(req, "%15s %511s", method, path) != 2) {
        send_error(client, 400, "invalid request line");
        return;
    }

    header_end = find_header_end(req);
    if (header_end) {
        body = header_end + 4;
    }

    route_request(client, method, path, body);
}

int main(void) {
    WSADATA wsa;
    SOCKET server_fd;
    struct sockaddr_in addr;
    char db_err[256];

    if (!backend_db_init("data/app.db", db_err, sizeof(db_err))) {
        fprintf(stderr, "[backend] DB init failed: %s\n", db_err);
        return 1;
    }

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "[backend] WSAStartup failed\n");
        return 1;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        fprintf(stderr, "[backend] socket failed\n");
        WSACleanup();
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(SERVER_PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
        fprintf(stderr, "[backend] bind failed\n");
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    if (listen(server_fd, 16) == SOCKET_ERROR) {
        fprintf(stderr, "[backend] listen failed\n");
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    printf("[backend] server listening on http://localhost:%d\n", SERVER_PORT);

    while (1) {
        SOCKET client = accept(server_fd, NULL, NULL);
        if (client == INVALID_SOCKET) {
            continue;
        }
        handle_client(client);
        closesocket(client);
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}
