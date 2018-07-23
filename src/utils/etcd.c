#include "etcd.h"

#include <curl/curl.h>

#include "log.h"
#include "../../3rd-deps/jsmn/jsmn.h"
#include "../../3rd-deps/base64/b64.h"

void free_string(res_string *s) {
    if (s != NULL) {
        free(s->ptr);
        free(s);
    }
}

void init_string(res_string *s) {
    s->len = 0;
    s->ptr = malloc(s->len + 1);
    if (s->ptr == NULL) {
        fprintf(stderr, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }
    s->ptr[0] = '\0';
}


int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
}

char *json_token_to_str(const char *json, jsmntok_t *tok) {
    char *str = malloc(sizeof(char) * (tok->end - tok->start + 1));
    sprintf(str, "%.*s", tok->end - tok->start,
            json + tok->start);
    return str;
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, res_string *s) {
    size_t new_len = s->len + size * nmemb;
    s->ptr = realloc(s->ptr, new_len + 1);
    if (s->ptr == NULL) {
        fprintf(stderr, "realloc() failed\n");
        exit(EXIT_FAILURE);
    }
    memcpy(s->ptr + s->len, ptr, size * nmemb);
    s->ptr[new_len] = '\0';
    s->len = new_len;

    return size * nmemb;
}

size_t dummy_write(void *ptr, size_t size, size_t nmemb, res_string *s) {
    return size * nmemb;
}

char *etcd_grant(char *etcd_url, int ttl) {
    CURL *hnd = curl_easy_init();
    res_string *s = malloc(sizeof(res_string));
    init_string(s);

    char full_url[1024];
    sprintf(full_url, "%s/v3alpha/lease/grant", etcd_url);
    curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(hnd, CURLOPT_URL, full_url);
    log_info("etcd etcd_grant url: %s\n", full_url);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);
    sprintf(full_url, "{\"TTL\":\"%d\"}", ttl);
    curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, full_url);
    log_info("etcd grant json: %s\n", full_url);
    curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(hnd, CURLOPT_WRITEDATA, s);

    CURLcode ret = curl_easy_perform(hnd);

    char *results = NULL;
    if (ret == CURLE_OK) {
        log_info("response body: %s", s->ptr);
        jsmn_parser parser;
        jsmntok_t t[1024];
        jsmn_init(&parser);

        int r = jsmn_parse(&parser, s->ptr, s->len, t, sizeof(t) / sizeof(t[0]));
        log_info("tokens: %d", r);
        if (r < 0) {
            log_fatal("error while parsing json: error code: %d, json_len: %d, json_str: %s", r, s->len, s->ptr);
        }
        for (int i = 0; i < r; i++) {
            if ((t[i].end - t[i].start == 2) && jsoneq(s->ptr, &t[i], "ID") == 0) {
                results = json_token_to_str(s->ptr, &t[i + 1]);//found ID
                break;
            }
        }
    }
    free_string(s);
    curl_easy_cleanup(hnd);
    curl_global_cleanup();
    return results;
}

int etcd_put(char *etcd_url, char *key, char *leaseID) {
    CURL *hnd = curl_easy_init();
    res_string *s = malloc(sizeof(res_string));
    init_string(s);

    char full_url[1024];
    sprintf(full_url, "%s/v3alpha/kv/put", etcd_url);
    curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(hnd, CURLOPT_URL, full_url);
    log_info("etcd put url: %s\n", full_url);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);
    char *encoded_key = b64_encode((unsigned char *) key, strlen(key));
    log_info("raw_key: %s, encoded_key: %s", key, encoded_key);
    sprintf(full_url, "{\"key\": \"%s\", \"value\": \"\", \"lease\":\"%s\"}", encoded_key, leaseID);
    curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, full_url);
    log_info("etcd put json: %s\n", full_url);
    curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, dummy_write);
    curl_easy_setopt(hnd, CURLOPT_WRITEDATA, NULL);
    CURLcode ret = curl_easy_perform(hnd);
    curl_easy_cleanup(hnd);
    curl_global_cleanup();
    return ret;
}

int etcd_get(char *etcd_url, char *key, char **server_addr, int *results_size) {
    CURL *hnd = curl_easy_init();
    res_string *s = malloc(sizeof(res_string));
    init_string(s);

    char full_url[1024];
    sprintf(full_url, "%s/v3alpha/kv/range", etcd_url);
    curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(hnd, CURLOPT_URL, full_url);
    log_info("etcd get url: %s\n", full_url);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);
    char *encoded_key = b64_encode((unsigned char *) key, strlen(key));
    sprintf(full_url, "{\"key\": \"%s\", \"range_end\":\"eg==\", \"sort_order\":1}", encoded_key);
    curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, full_url);
    log_info("etcd get json: %s\n", full_url);
    curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(hnd, CURLOPT_WRITEDATA, s);

    CURLcode ret = curl_easy_perform(hnd);
    *results_size = 0;
    if (ret == CURLE_OK) {
        log_info("response body: %s", s->ptr);
        jsmn_parser parser;
        jsmntok_t t[1024];
        jsmn_init(&parser);

        int r = jsmn_parse(&parser, s->ptr, s->len, t, sizeof(t) / sizeof(t[0]));
        log_info("tokens: %d", r);
        if (r < 0) {
            log_fatal("error while parsing json: error code: %d, json_len: %d, json_str: %s", r, s->len, s->ptr);
        }
        int size = 0;
        for (int i = 0; i < r; i++) {
            if ((t[i].end - t[i].start == 3) && jsoneq(s->ptr, &t[i], "key") == 0) {
                char *raw_str = json_token_to_str(s->ptr, &t[i + 1]);
                //log_info("raw str: %s", raw_str);
                char *decoded_str = (char *) b64_decode(raw_str, strlen(raw_str));
                //log_info("decoded str: %s", decoded_str);
                server_addr[size] = decoded_str;
                i++;
                size++;
            }
        }
        *results_size = size;
    }
    free_string(s);
    curl_easy_cleanup(hnd);
    curl_global_cleanup();
    return ret;
}

int etcd_keep_alive(char *etcd_url, char *leaseID) {
    CURL *hnd = curl_easy_init();
    res_string *s = malloc(sizeof(res_string));
    init_string(s);

    char full_url[1024];
    sprintf(full_url, "%s/v3alpha/lease/keepalive", etcd_url);
    curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(hnd, CURLOPT_URL, full_url);
    log_info("etcd keep-alive url: %s\n", full_url);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);

    sprintf(full_url, "{\"ID\":\"%s\"}", leaseID);

    curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, full_url);
    log_info("etcd keep-alive json: %s\n", full_url);
    curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, dummy_write);
    CURLcode ret = curl_easy_perform(hnd);
    curl_easy_cleanup(hnd);
    curl_global_cleanup();
    return ret;
}