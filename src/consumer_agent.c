#include "consumer_agent.h"

#include <unistd.h>

#include "utils/log.h"
#include "utils/utils.h"
#include "utils/etcd.h"
#include "../3rd-deps/picohttpparser/picohttpparser.h"

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#define DEFAULT_BACKLOG 256
#define DEFAULT_PACKAGE_SIZE 4096
#define DEFAULT_RESPONSE_SIZE 4096
#define MAXIMUM_CONN_WARMUP_COUNT 256
#define MAXIMUM_CONN_COUNT 1024
#define MAXIMUM_ENDPOINT_SERVERS 16

int64_t start_time;

int num_endpoint_servers;
struct sockaddr_in addr;
int server_port;
char **raw_server_string;
char **endpoint_server_addr_str;
struct sockaddr_in endpoint_sockaddr[MAXIMUM_ENDPOINT_SERVERS];
int *endpoint_server_port;
int *endpoint_server_scale;//0->small, 1->medium, 2->large

load_balancer_t *global_lb;
uv_mutex_t lb_lock;

int global_conn_remain_count;
uv_mutex_t global_conn_count_mutex;

uv_loop_t *main_loop;
tcp_server_t *tcp_servers[EVENT_LOOP_NUM];


void make_connection_to_agent_(int which_endpoint, tcp_server_t *tcp_server);

void connect_to_provider_agent_long_live(tcp_server_t *tcp_server);

tcp_server_t *init_tcp_server_t(int tid) {
    tcp_server_t *tcp_server = malloc(sizeof(tcp_server_t));
    tcp_server->work_event_loop = uv_loop_new();
    tcp_server->req_id = tid;
    memset(tcp_server->http_req_dict, 0, sizeof(http_req_t *) * HASH_SLOT_NUM);
    return tcp_server;
}

void init_consumer(int server_p, char *etcd_url) {
    setenv("UV_THREADPOOL_SIZE", "4", 1);
    server_port = server_p;
    raw_server_string = (char **) malloc(sizeof(char *) * MAXIMUM_ENDPOINT_SERVERS);
    endpoint_server_addr_str = (char **) malloc(sizeof(char *) * MAXIMUM_ENDPOINT_SERVERS);
    endpoint_server_port = (int *) malloc(sizeof(int) * MAXIMUM_ENDPOINT_SERVERS);
    endpoint_server_scale = (int *) malloc(sizeof(int) * MAXIMUM_ENDPOINT_SERVERS);
    //dubbomesh/com.some.package.IHelloService/192.168.100.100:2000

    int r = etcd_get(etcd_url, "/dubbomesh/com.alibaba.dubbo.performance.demo.provider.IHelloService/",
                     raw_server_string,
                     &num_endpoint_servers);
    if (r != 0) {
        log_fatal("cannot connected to etcd server: %s", etcd_url);
        exit(-1);
    }
    if (num_endpoint_servers == 0) {
        log_fatal("cannot find any endpoint server, exiting");
        exit(-1);
    }
    for (int i = 0; i < num_endpoint_servers; i++) {
        log_info("raw_server_str: %s", raw_server_string[i]);
        endpoint_server_addr_str[i] = malloc(sizeof(char) * 256);
        str_to_server_uri(raw_server_string[i], endpoint_server_addr_str[i], &endpoint_server_port[i],
                          &endpoint_server_scale[i]);
        log_info("endpoint server addr: %s, port: %d, scale %d", endpoint_server_addr_str[i], endpoint_server_port[i],
                 endpoint_server_scale[i]);
        uv_ip4_addr(endpoint_server_addr_str[i], endpoint_server_port[i], &endpoint_sockaddr[i]);
    }
    uv_ip4_addr("0.0.0.0", server_port, &addr);

    // init global load balancer
    global_lb = init_load_balancer(num_endpoint_servers);
    uv_mutex_init(&lb_lock);

    global_conn_remain_count = 0;
    uv_mutex_init(&global_conn_count_mutex);
}

http_req_t *init_http_req_t(size_t size) {
    http_req_t *req = malloc(sizeof(http_req_t));
    req->client = NULL;
    req->raw_array = (char *) malloc(sizeof(char) * size);
    req->raw_array_size = 0;
    req->raw_array_max_size = size;
    req->request_length = -1;
    req->req_id = -1;
    req->response_package = (char *) malloc(sizeof(char) * DEFAULT_PACKAGE_SIZE);
    return req;
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    http_req_t *http_req = handle->data;
    buf->base = http_req->raw_array + http_req->raw_array_size;
    buf->len = http_req->raw_array_max_size - http_req->raw_array_size;
}

void alloc_buffer_new(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    buf->base = malloc(sizeof(char) * suggested_size);
    buf->len = suggested_size;
}

void alloc_buffer_to_agent(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    endpoint_server_t *endpoint_server = handle->data;
    buf->base = endpoint_server->raw_array + endpoint_server->size_of_raw_array;
    buf->len = endpoint_server->raw_array_max_length - endpoint_server->size_of_raw_array;
}

void free_http_req(http_req_t *http_req) {
    if (http_req != NULL) {
        if (http_req->client)
            free(http_req->client);
        free(http_req->raw_array);
        free(http_req->response_package);
        free(http_req);
    }
}

void close_cb(uv_handle_t *handle) {
    http_req_t *http_req = (http_req_t *) handle->data;
    free_http_req(http_req);

}

void after_write_to_provider_agent(uv_write_t *handle, int status) {
    free(handle);
}

void send_to_provider_agent(int which_endpoint, http_req_t *http_req, char *send_buffer, int send_buff_len) {
    tcp_server_t *tcp_server = http_req->tcp_server;
    endpoint_server_t *endpoint_server = tcp_server->endpoint_servers[which_endpoint];

    uv_buf_t local_buf = uv_buf_init(send_buffer, send_buff_len);
    uv_write_t *write_req = malloc(sizeof(uv_write_t));
    write_req->data = http_req;

    http_req->tcp_server->http_req_dict[http_req->req_id % HASH_SLOT_NUM] = http_req;
    uv_write(write_req, (uv_stream_t *) endpoint_server->tcp_stream, &local_buf, 1, after_write_to_provider_agent);
}

void c_read_connection_data(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    http_req_t *http_req = client->data;
    tcp_server_t *tcp_server = http_req->tcp_server;

    if (nread > 0) {
        http_req->raw_array_size += nread;
        // 1st: parse header
        const char *method, *path;
        int minor_version;
        struct phr_header headers[16];
        size_t method_len, path_len, num_headers;
        num_headers = 16;
        if (http_req->request_length < 0) {
            http_req->request_length = phr_parse_request(http_req->raw_array, http_req->raw_array_size, &method,
                                                         &method_len, &path, &path_len,
                                                         &minor_version, headers, &num_headers, 0);
        }
        if (http_req->request_length < 0)
            return;
        //log_info("%.*s", http_req->raw_array_size, http_req->raw_array);
        int content_length = atoi(headers[0].value);// this can be fixed according to a particular http client

        http_req->content_length = content_length;
        if (http_req->raw_array_size < content_length + http_req->request_length)
            return;

        uv_mutex_lock(&lb_lock);
        int which_endpoint = select_endpoint_time_based(global_lb, http_req);
        uv_mutex_unlock(&lb_lock);

        http_req->req_id = tcp_server->req_id;
        tcp_server->req_id += EVENT_LOOP_NUM;
        //4bytes package length, 8 bytes req_id, http body
        int raw_array_start_offset = http_req->request_length - 12;
        int_to_four_char(content_length, http_req->raw_array + raw_array_start_offset);
        long_to_8bytes(http_req->req_id, http_req->raw_array + raw_array_start_offset + 4);

        send_to_provider_agent(which_endpoint, http_req, http_req->raw_array + raw_array_start_offset,
                               12 + content_length);

        // 3rd: copy remain buff to the beginning of the buffer
        http_req->raw_array_size = 0;
        http_req->request_length = -1;
    } else if (nread < 0) {
        if (nread != UV_EOF) { log_error("Read error %s\n", uv_strerror(nread)); }
        log_info("err happens in http");
        uv_close((uv_handle_t *) client, close_cb);

        uv_mutex_lock(&global_conn_count_mutex);
        global_conn_remain_count--;
        uv_mutex_unlock(&global_conn_count_mutex);
    }
}

void c_dummy_read_connection_data(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    if (nread < 0) {
        uv_close((uv_handle_t *) client, NULL);
    }
    free(buf->base);
}

void on_new_connection(uv_stream_t *server, int status) {
    if (status != 0) {
        log_error("New connection error %s\n", uv_strerror(status));
        return;
    }
    tcp_server_t *tcp_server = server->data;

    uv_tcp_t *client = (uv_tcp_t *) malloc(sizeof(uv_tcp_t));
    uv_tcp_init(tcp_server->work_event_loop, client);
    uv_tcp_nodelay(client, 1);
    uv_tcp_keepalive(client, 1, 60);
    //log_info("start accept a connection");
    int64_t cur_time = get_wall_time();
    int conn_limit = (cur_time - start_time > 30000000 ? MAXIMUM_CONN_COUNT : MAXIMUM_CONN_WARMUP_COUNT);
    if (uv_accept(server, (uv_stream_t *) client) == 0) {
        int tmp;
        uv_mutex_lock(&global_conn_count_mutex);
        tmp = global_conn_remain_count;
        if (tmp < conn_limit)
            global_conn_remain_count++;
        uv_mutex_unlock(&global_conn_count_mutex);
        if (tmp < conn_limit) {
//            log_info("read start a conn, global conn remain count %d", tmp);
            http_req_t *req = init_http_req_t(DEFAULT_PACKAGE_SIZE * 2);
            req->tcp_server = tcp_server;
            req->client = client;
            client->data = (void *) req;
            uv_read_start((uv_stream_t *) client, alloc_buffer, c_read_connection_data);
        } else {
            uv_read_start((uv_stream_t *) client, alloc_buffer_new, c_dummy_read_connection_data);
        }
    } else {
        uv_close((uv_handle_t *) client, NULL);
    }
}

void after_write_cb(uv_write_t *write_req, int status) {
    free(write_req);
}

void reconnection_to_agent(uv_handle_t *handle) {
    endpoint_server_t *server = handle->data;
    tcp_server_t *tcp_server = server->tcp_server;
    log_info("retry to connect to endpoint server %d", server->which);
    make_connection_to_agent_(server->which, tcp_server);
}

void c_read_from_provide_agents(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    endpoint_server_t *server = client->data;

    if (nread > 0) {
        server->size_of_raw_array += nread;
        tcp_server_t *tcp_server = server->tcp_server;
        for (;;) {
            if (server->size_of_raw_array < 12) { return; }
            int length_body = four_char_to_int(server->raw_array);
            int last_length = length_body + 12;
            if (server->size_of_raw_array < last_length) { return; }

            //log_info("res: %.*s", length_body, server->raw_array + 12);

            int64_t req_id = bytes8_to_long(server->raw_array + 4);
            //log_info("res req id: %lld", req_id);
            http_req_t *http_conn = tcp_server->http_req_dict[req_id % HASH_SLOT_NUM];
            if (http_conn != NULL) {
                uv_mutex_lock(&lb_lock);
                finish_endpoint_task_time_based(global_lb, server->which, http_conn);
                uv_mutex_unlock(&lb_lock);

                memcpy(http_conn->response_package, server->raw_array + 12, length_body);

                uv_buf_t res_buf = uv_buf_init(http_conn->response_package, length_body);
                uv_write_t *write_req = (uv_write_t *) malloc(sizeof(uv_write_t));
                write_req->data = http_conn;
                uv_write(write_req, (uv_stream_t *) http_conn->client, &res_buf, 1, after_write_cb);
            } else {
                // handle time-out influencing remain
                uv_mutex_lock(&lb_lock);
                global_lb->history_diff_time_lst[server->which]->remain--;
                uv_mutex_unlock(&lb_lock);
            }

            // 2nd: make the buffer correct, handling ticked packets
            server->size_of_raw_array = server->size_of_raw_array - last_length;
            memmove(server->raw_array, server->raw_array + last_length, server->size_of_raw_array);

        }
    } else if (nread < 0) {
        if (nread != UV_EOF) {
            log_error("Read error %s\n", uv_strerror(nread));
            log_fatal("error while reading from provider agent %d", ((endpoint_server_t *) client->data)->which);
            exit(-1);
        }
        server->conn_status = -1;
        uv_close((uv_handle_t *) client, reconnection_to_agent);
    }
}

void start_listen(tcp_server_t *tcp_server) {
    log_info("start listening");
    uv_tcp_init_ex(tcp_server->work_event_loop, &(tcp_server->server), AF_INET);
    uv_os_fd_t fd;
    uv_fileno((uv_handle_t *) &(tcp_server->server), &fd);
    int optval = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    uv_tcp_bind(&(tcp_server->server), (const struct sockaddr *) &addr, 0);
    uv_tcp_nodelay(&(tcp_server->server), 1);
    tcp_server->server.data = tcp_server;
    int r = uv_listen((uv_stream_t *) &(tcp_server->server), DEFAULT_BACKLOG, on_new_connection);

    if (r) {
        log_fatal("Listen error %s\n", uv_strerror(r));
        exit(-1);
    }
}

void c_after_connect_to_a_agent(uv_connect_t *connection, int status) {
    endpoint_server_t *server = connection->data;
    tcp_server_t *tcp_server = server->tcp_server;
    if (status == 0) {
        log_info("connected to provider %d successful", server->which);
        server->conn_status = 0;
        server->tcp_stream->data = server;
        log_info("server pointer address: %x", server);
        log_info("c_after_connect_to_a_agent which server: %d", server->which);
        uv_read_start((uv_stream_t *) server->tcp_stream, alloc_buffer_to_agent, c_read_from_provide_agents);
        tcp_server->endpoint_connection_count++;
        if (tcp_server->endpoint_connection_count == num_endpoint_servers) {
            start_listen(tcp_server);
        }

    } else {
        log_info("connected to provider %d failed", server->which);
        sleep(3);
        log_info("retry to connect provider %d", server->which);
        make_connection_to_agent_(server->which, tcp_server);
    }
}

void make_connection_to_agent_(int which_endpoint, tcp_server_t *tcp_server) {
    int i = which_endpoint;
    if (tcp_server->endpoint_servers[i]->conn_status != 0) {
        log_info("init connection to provider %d", i);
        uv_connect_t *conn = malloc(sizeof(uv_connect_t));
        if (tcp_server->endpoint_servers[i]->conn)
            free(tcp_server->endpoint_servers[i]->conn);
        tcp_server->endpoint_servers[i]->conn = conn;
        uv_tcp_t *client = malloc(sizeof(uv_tcp_t));
        if (tcp_server->endpoint_servers[i]->tcp_stream)
            free(tcp_server->endpoint_servers[i]->tcp_stream);
        tcp_server->endpoint_servers[i]->tcp_stream = client;

        uv_tcp_init(tcp_server->work_event_loop, client);
        uv_tcp_nodelay(client, 1);
        conn->data = tcp_server->endpoint_servers[i];
        uv_tcp_keepalive(client, 1, 60);
        log_info("start to connect endpoint %d", i);
        uv_tcp_connect(conn, client,
                       (const struct sockaddr *) &endpoint_sockaddr[tcp_server->endpoint_servers[i]->which],
                       c_after_connect_to_a_agent);
    }
}

void connect_to_provider_agent_long_live(tcp_server_t *tcp_server) {
    //connect to provider agents
    log_info("init connections to provider agents");
    for (int i = 0; i < num_endpoint_servers; i++) {
        make_connection_to_agent_(i, tcp_server);
    }
}

void start_work_event_loop(void *args) {
    tcp_server_t *tcp_server = (tcp_server_t *) args;
    log_info("start work event loop");

    for (int i = 0; i < num_endpoint_servers; i++) {
        endpoint_server_t *server = malloc(sizeof(endpoint_server_t));
        server->which = i;
        server->conn_status = -1;
        server->raw_array = malloc(sizeof(char) * DEFAULT_RESPONSE_SIZE);
        server->size_of_raw_array = 0;
        server->raw_array_max_length = DEFAULT_RESPONSE_SIZE;
        server->tcp_server = tcp_server;
        server->conn = NULL;
        server->tcp_stream = NULL;
        log_info("start_event_loop, tcp_server addr: 0x%x", tcp_server);
        tcp_server->endpoint_servers[i] = server;
    }

    tcp_server->endpoint_connection_count = 0;
    connect_to_provider_agent_long_live(tcp_server);

    uv_run(tcp_server->work_event_loop, UV_RUN_DEFAULT);
}

int consumer(int server_port, char *etcd_url) {
    log_info("libuv version: %d.%d.%d%s", UV_VERSION_MAJOR, UV_VERSION_MINOR, UV_VERSION_PATCH, UV_VERSION_SUFFIX);
    get_cpu_info();
    init_consumer(server_port, etcd_url);
    main_loop = uv_default_loop();
    start_time = get_wall_time();
    for (int i = 0; i < EVENT_LOOP_NUM; i++) {
        tcp_servers[i] = init_tcp_server_t(i);
    }
    uv_thread_t work_tid[EVENT_LOOP_NUM];

    for (int i = 0; i < EVENT_LOOP_NUM; i++) {
        uv_thread_create(&work_tid[i], start_work_event_loop, tcp_servers[i]);
    }

    //if main loop finish, wait for other event loops
    for (int i = 0; i < EVENT_LOOP_NUM; i++) {
        uv_thread_join(&work_tid[i]);
    }
    return 0;
}