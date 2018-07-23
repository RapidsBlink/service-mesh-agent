#pragma once

#include <uv.h>

typedef struct p_tcp_server_struct_t p_tcp_server_t;

typedef struct {
    uv_tcp_t *tcp_conn_1;
    uv_tcp_t *tcp_conn_2;
    uv_connect_t *dubbo_connection;
    p_tcp_server_t *p_tcp_server;
    char *raw_buf1;
    char *raw_buf2;
    size_t raw_buf1_size, raw_buf2_size;
    size_t max_buffer_size;
} tcp_relay_t;

struct p_tcp_server_struct_t {
    uv_loop_t *work_event_loop;
    uv_tcp_t agent_server;
    int tid;
};

int provider(char *provider_type, int server_port, int dubbo_port, char *etcd_url);
