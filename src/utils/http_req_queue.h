#pragma once

#include <glob.h>

struct http_req;
typedef struct http_req http_req_t;

typedef struct {
    http_req_t **buffer;
    int capacity;
    int head, tail;
    size_t generation;
}http_req_queue_t;

http_req_queue_t *init_http_req_queue_t(int default_size);

int http_req_queue_push(http_req_queue_t* queue, http_req_t *http_req);

http_req_t* http_req_queue_pull(http_req_queue_t *queue);
