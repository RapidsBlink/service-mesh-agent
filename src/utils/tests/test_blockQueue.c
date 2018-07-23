#include "../blockQueue.h"
#include "../../consumer_agent.h"
#include <stdlib.h>
#include <unistd.h>

void *func1(void *bqi) {
    blockQueue *bq = (blockQueue *) bqi;

    int xor = 0xef;
    for (int i = 0; i < 5000; i++) {
        http_req_t *q = malloc(sizeof(http_req_t));
        q->request_length = 8 ^ xor;
        int r = bq_push(bq, q);
        if (r != 0) {
            fprintf(stderr, "block queue is full");
        }
        usleep(10);
    }
    return NULL;
}

void *func2(void *bqi) {
    blockQueue *bq = (blockQueue *) bqi;
    int xor = 0xfe;
    for (int i = 0; i < 5000; i++) {
        http_req_t *q = malloc(sizeof(http_req_t));
        q->request_length = 8 ^ xor;
        int r = bq_push(bq, q);
        if (r != 0) {
            fprintf(stderr, "block queue is full\n");
        }
        usleep(10);
    }
    return NULL;
}

void *func3(void *bqi) {
    blockQueue *bq = (blockQueue *) bqi;

    for (int i = 0; i < 10000; i++) {
        http_req_t *req = bq_pull(bq);
        if ((req->request_length ^ 0xef) == 8 || (req->request_length ^ 0xfe) == 8) {
            //fprintf(stderr, "ok\n");
        } else {
            fprintf(stderr, "verify error\n");
        }
    }

    return NULL;

}

int main() {

    blockQueue *bq = init_blockQueue(5);

    pthread_t p1, p2, p3;
    pthread_create(&p3, NULL, func1, (void *) bq);
    pthread_create(&p1, NULL, func2, (void *) bq);
    pthread_create(&p2, NULL, func3, (void *) bq);


    pthread_join(p1, NULL);
    pthread_join(p2, NULL);
    pthread_join(p3, NULL);
//    func1(bq);
//    func2(bq);
//    func3(bq);
    return 0;
}