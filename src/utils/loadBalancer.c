#include <stdlib.h>

#include "loadBalancer.h"

#include "../consumer_agent.h"
#include "utils.h"

int direct_send_limit[3] = {8, 8, 8};
int penalty_send_limit[3] = {140, 200, 200};

history_diff_time_t *init_history_time() {
    history_diff_time_t *res = malloc(sizeof(history_diff_time_t));
    for (int i = 0; i < HISTORY_WINDOW_SIZE; i++) { res->diff_time_history[i] = 50000; }
    res->diff_time_sum = 50000 * HISTORY_WINDOW_SIZE;
    res->head = 0;
    res->tail = HISTORY_WINDOW_SIZE - 1;

    res->remain = 0;
    return res;
}

load_balancer_t *init_load_balancer(int num_of_ends) {
    load_balancer_t *this = malloc(sizeof(load_balancer_t));
    this->num_of_ends_ = num_of_ends;
    for (int i = 0; i < num_of_ends; i++) { this->history_diff_time_lst[i] = init_history_time(); }
    return this;
}

inline int64_t estimate_cost(load_balancer_t *this, int index) {
    history_diff_time_t *history_diff_time = this->history_diff_time_lst[index];
    if (history_diff_time->remain >= penalty_send_limit[index]) {
        return INT64_MAX;
    }
    if (history_diff_time->remain < direct_send_limit[index]) {
        return  0;
    }
    return history_diff_time->diff_time_sum;
}

int select_endpoint_time_based(load_balancer_t *this, http_req_t *http_req) {
    int min_idx = this->num_of_ends_ - 1;
    http_req->ts_req_beg = get_wall_time();
    int64_t min_val = estimate_cost(this, min_idx);
    for (int i = this->num_of_ends_ - 2; i >= 0; i--) {
        int64_t tmp = estimate_cost(this, i);
        if (tmp < min_val) {
            min_idx = i;
            min_val = tmp;
        }
    }
    this->history_diff_time_lst[min_idx]->remain++;
    return min_idx;
}

void finish_endpoint_task_time_based(load_balancer_t *this, int end_point_idx, http_req_t *http_req) {
    this->history_diff_time_lst[end_point_idx]->remain--;
    int64_t new_diff_time = get_wall_time() - http_req->ts_req_beg;

    history_diff_time_t *history_diff_time = this->history_diff_time_lst[end_point_idx];
    int64_t oldest_diff = history_diff_time->diff_time_history[history_diff_time->head];
    history_diff_time->head++;
    history_diff_time->head %= HISTORY_WINDOW_SIZE;
    history_diff_time->diff_time_sum -= oldest_diff;

    history_diff_time->tail++;
    history_diff_time->tail %= HISTORY_WINDOW_SIZE;
    history_diff_time->diff_time_history[history_diff_time->tail] = new_diff_time;
    history_diff_time->diff_time_sum += new_diff_time;
}