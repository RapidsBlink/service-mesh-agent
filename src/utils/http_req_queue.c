#include "http_req_queue.h"
#include <stdlib.h>

http_req_queue_t *init_http_req_queue_t(int default_size) {
    http_req_queue_t *queue = malloc(sizeof(http_req_queue_t));
    queue->buffer = malloc(sizeof(http_req_t *) * default_size);
    queue->head = queue->tail = 0;
    queue->capacity = default_size;
    queue->generation = 0;
    return queue;
}

int _http_req_queue_test_empty(http_req_queue_t *queue){
    if(queue->generation == 0 && queue->head == queue->tail)
        return 1;
    return 0;
}
int _http_req_queue_test_full(http_req_queue_t *queue){
    if(queue->generation == 1 && queue->head == queue->tail)
        return 1;
    return 0;
}

int http_req_queue_push(http_req_queue_t *queue, http_req_t *http_req) {
    if(_http_req_queue_test_full(queue)){
        return 1;//if full, drop
    }
    queue->buffer[queue->tail] = http_req;
    queue->tail++;
    if(queue->tail > queue->capacity){
        queue->tail = 0;
        queue->generation = 1;
    }
    return 0;
}

http_req_t *http_req_queue_pull(http_req_queue_t *queue) {
    if(_http_req_queue_test_empty(queue))
        return NULL;
    http_req_t *req = queue->buffer[queue->head];
    queue->head++;
    if(queue->head > queue->capacity){
        queue->head = 0;
        queue->generation = 0;
    }
    return req;
}
