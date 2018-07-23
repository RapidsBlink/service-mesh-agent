#pragma once

#include <uv.h>

#include "../3rd-deps/uthash/include/uthash.h"
#include "utils/loadBalancer.h"

#define HASH_SLOT_NUM (8192)

struct tcp_server_struct;
typedef struct tcp_server_struct tcp_server_t;

typedef struct http_req {
    uv_tcp_t *client;//this is the connection come from consumer

    char *raw_array;
    size_t raw_array_size;
    size_t raw_array_max_size;
    int request_length;
    int content_length;
    int64_t req_id;

    char *response_package;
    tcp_server_t *tcp_server;

    int64_t ts_req_beg;
} http_req_t;

typedef struct {
    int which;
    char *raw_array;
    size_t size_of_raw_array;
    size_t raw_array_max_length;
    uv_tcp_t *tcp_stream;
    uv_connect_t *conn;
    int conn_status;
    tcp_server_t *tcp_server;
} endpoint_server_t;

struct tcp_server_struct {
    uv_loop_t *work_event_loop;
    uv_tcp_t server;
    int endpoint_connection_count;
    int64_t req_id;//uint64
    endpoint_server_t *endpoint_servers[16];
    http_req_t *http_req_dict[HASH_SLOT_NUM];
};

int consumer(int server_port, char *etcd_url);
