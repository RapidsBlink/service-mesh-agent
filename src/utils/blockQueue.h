#pragma once

#include <uv.h>
#include "../consumer_agent.h"

typedef struct {
    uv_mutex_t *mutex;
    uv_cond_t *cond;
    size_t head, tail, max_size;
    int tail_ahead;
    http_req_t **http_req_array;
} blockQueue;

blockQueue *init_blockQueue(int size);

void free_blockQueue(blockQueue *bq);

int test_full(blockQueue *bq);

int test_empty(blockQueue *bq);

int bq_push(blockQueue *bq, http_req_t *http_req);

http_req_t *bq_pull(blockQueue *bq);