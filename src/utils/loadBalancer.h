#pragma once

#include <uv.h>
#include "../config.h"

struct http_req;
typedef struct http_req http_req_t;

typedef struct {
    int64_t diff_time_sum;
    int head;
    int tail;
    int remain;
    int64_t diff_time_history[HISTORY_WINDOW_SIZE];
} history_diff_time_t;

typedef struct {
    int num_of_ends_;
    history_diff_time_t *history_diff_time_lst[16];
} load_balancer_t;

load_balancer_t *init_load_balancer(int num_of_ends);

history_diff_time_t *init_history_time();

int select_endpoint_time_based(load_balancer_t *this, http_req_t *http_req);

void finish_endpoint_task_time_based(load_balancer_t *this, int end_point_idx, http_req_t *http_req);

