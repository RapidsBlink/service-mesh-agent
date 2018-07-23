#include <stdlib.h>
#include <unistd.h>

#include "provider_agent.h"
#include "utils/log.h"
#include "utils/etcd.h"
#include "utils/utils.h"
#include "config.h"
#include "utils/tokenizer.h"

#define P_DEFAULT_BACKLOG 256 // see https://linux.die.net/man/2/listen
#define P_DEFAULT_PACKAGE_SIZE 4096
#define P_DEFAULT_LEASE_TTL 600

static const char *response_template = "HTTP/1.1 200 OK\r\nContent-Length:   \r\n\r\n";
static const int response_template_len = 39;

uv_loop_t *main_loop;
struct sockaddr_in addr;
struct sockaddr_in dubbo_addr;
struct addrinfo hints;
uv_getaddrinfo_t resolver;
char provider_scale;

int server_port;
int dubbo_port;
char *etcd_url;
char *lease_id;

uv_thread_t keep_alive_tid;
p_tcp_server_t *p_tcp_servers[P_EVENT_LOOP_NUM];

void test_dubbo_connection();

void connect_to_dubbo_long_live(tcp_relay_t *tcp_relay);

tcp_relay_t *init_tcp_relay_t(size_t size) {
    tcp_relay_t *relay = (tcp_relay_t *) malloc(sizeof(tcp_relay_t));
    relay->tcp_conn_1 = NULL;
    relay->tcp_conn_2 = NULL;
    relay->dubbo_connection = NULL;
    relay->raw_buf1 = malloc(sizeof(char) * size);
    relay->raw_buf2 = malloc(sizeof(char) * size);
    relay->max_buffer_size = size;
    relay->raw_buf1_size = 0;
    relay->raw_buf2_size = 0;
    return relay;
}

p_tcp_server_t *init_p_tcp_server_t(int tid) {
    p_tcp_server_t *tcp_server = malloc(sizeof(p_tcp_server_t));
    tcp_server->work_event_loop = uv_loop_new();
    tcp_server->tid = tid;
    return tcp_server;
}

void free_tcp_relay_t(tcp_relay_t *tcp_relay) {
    free(tcp_relay->tcp_conn_1);
    free(tcp_relay->tcp_conn_2);
    free(tcp_relay->dubbo_connection);
    free(tcp_relay);
}

void keep_alive_thread_work(void *arg) {
    char *ID = (char *) arg;
    for (;;) {
        sleep((P_DEFAULT_LEASE_TTL - 10) > 3 ? (P_DEFAULT_LEASE_TTL - 10)
                                             : 3);//sleep for (TTL-10) to update lease time to etcd
        log_info("send heartbeat to etcd.");
        etcd_keep_alive(etcd_url, ID);
    }
}

void start_keep_alive_etcd(char *ID) {
    uv_thread_create(&keep_alive_tid, keep_alive_thread_work, (void *) ID);
}

void on_resolved(uv_getaddrinfo_t *resolver, int status, struct addrinfo *res) {
    if (status != 0) {
        log_fatal("error while resolving ip addr, reason: %s", uv_strerror(status));
        exit(-1);
    }

    char addr[17] = {'\0'};
    uv_ip4_name((struct sockaddr_in *) res->ai_addr, addr, 16);
    log_info("ipaddr is %s", addr);

    //register with etcd
    lease_id = etcd_grant(etcd_url, P_DEFAULT_LEASE_TTL);// request a 300s grantID
    char buff[2048];
    sprintf(buff, "/dubbomesh/com.alibaba.dubbo.performance.demo.provider.IHelloService/%c:%s:%d", provider_scale, addr,
            server_port);
    etcd_put(etcd_url, buff, lease_id);
    log_info("register endpoint: %s finished", buff);
    start_keep_alive_etcd(lease_id);

    test_dubbo_connection();
}

void init_provider(char *provider_type, int server_p, int dubbo_p, char *etcd_u) {
    dubbo_port = dubbo_p;
    server_port = server_p;
    etcd_url = etcd_u;
    if (strcmp(provider_type, "provider-small") == 0) {
        provider_scale = '0';
    } else if (strcmp(provider_type, "provider-medium") == 0) {
        provider_scale = '1';
    } else if (strcmp(provider_type, "provider-large") == 0) {
        provider_scale = '2';
    } else {
        log_fatal("unknown provider scale %s", provider_type);
        exit(-1);
    }

    uv_ip4_addr("127.0.0.1", dubbo_port, &dubbo_addr);
    uv_ip4_addr("0.0.0.0", server_port, &addr);
    hints.ai_family = PF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = 0;
    log_info("resolve %s ip addr...", provider_type);
    int r = uv_getaddrinfo(main_loop, &resolver, on_resolved, provider_type, NULL, &hints);

    if (r) {
        log_fatal("getaddrinfo call error");
        exit(-1);
    }
}

void p_alloc_buffer_new(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    buf->base = malloc(sizeof(char) * suggested_size);
    buf->len = suggested_size;
}

void p_alloc_buffer1(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    tcp_relay_t *relay = handle->data;
    buf->base = relay->raw_buf1 + relay->raw_buf1_size;
    buf->len = relay->max_buffer_size - relay->raw_buf1_size;
}

void p_alloc_buffer2(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    tcp_relay_t *relay = handle->data;
    buf->base = relay->raw_buf2 + relay->raw_buf2_size;
    buf->len = relay->max_buffer_size - relay->raw_buf2_size;
}

void p_close_tcp_relay_connection2(uv_handle_t *handle) {
    log_info("connection 2 close successful");
    tcp_relay_t *tcp_relay = handle->data;
    free_tcp_relay_t(tcp_relay);
}

void p_close_tcp_relay_connection1(uv_handle_t *handle) {
    log_info("connection 1 close successful");
    tcp_relay_t *tcp_relay = handle->data;
    uv_close((uv_handle_t *) tcp_relay->tcp_conn_2, p_close_tcp_relay_connection2);
}

void free_write_req_without_data(uv_write_t *req, int status) {
    free(req);
}

void free_write_req_with_data(uv_write_t *req, int status) {
    free(req->data);
    free(req);
}

void p_on_read_results(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    //log_info("on p_on_read_results");
    tcp_relay_t *tcp_relay = client->data;
    if (nread > 0) {
        tcp_relay->raw_buf2_size += nread;
        for (;;) {
            if (tcp_relay->raw_buf2_size < 16)
                return;
            int length = four_char_to_int(tcp_relay->raw_buf2 + 12);
            //log_info("length %d", length);
            int last_length = 16 + length;
            if (tcp_relay->raw_buf2_size < last_length) { return; }
            //int64_t req_id = bytes8_to_long(tcp_relay->raw_buf2 + 4);
            //log_info("req_id %lld", req_id);
            //log_info("dubbo res: %.*s", length, tcp_relay->raw_buf2 + 16);
            if (tcp_relay->raw_buf2[3] == 20) {
                char *response_all = malloc(sizeof(char) * 4096);
                char *response_body = response_all + 12;

                int hash_length = length - 3;
                char *hash_res = tcp_relay->raw_buf2 + 18;

                int res_current_length = 0;

                memcpy(response_body, response_template, response_template_len);
                res_current_length += response_template_len;
                int_to_string(response_body + 33, hash_length);

                memcpy(response_body + res_current_length, hash_res, hash_length);
                res_current_length += hash_length;
                int_to_four_char(res_current_length, response_all);
                memcpy(response_all + 4, tcp_relay->raw_buf2 + 4, 8);

                uv_write_t *write_req = malloc(sizeof(uv_write_t));
                write_req->data = response_all;
                uv_buf_t local_buf = uv_buf_init(response_all, res_current_length + 12);
                //log_info("res: %.*s", res_current_length, response_body);
                uv_write(write_req, (uv_stream_t *) tcp_relay->tcp_conn_1, &local_buf, 1, free_write_req_with_data);
            }
            tcp_relay->raw_buf2_size -= last_length;
            memmove(tcp_relay->raw_buf2, tcp_relay->raw_buf2 + last_length, tcp_relay->raw_buf2_size);
        }
    } else if (nread < 0) {
        if (nread != UV_EOF) {
            log_error("Read error %s\n", uv_strerror(nread));
        }
        uv_close((uv_handle_t *) tcp_relay->tcp_conn_1, p_close_tcp_relay_connection1);
    }
}

void p_read_connection_data_http_req(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    //log_info("on p_read_connection_data");
    tcp_relay_t *tcp_relay = client->data;
    if (nread > 0) {
        tcp_relay->raw_buf1_size += nread;
        for (;;) {
            if (tcp_relay->raw_buf1_size < 12)
                return;

            int package_length = four_char_to_int(tcp_relay->raw_buf1);
            //log_info("package size %d", package_length);
            int last_length = 12 + package_length;
            if (tcp_relay->raw_buf1_size < last_length)
                return;

            int64_t req_id = bytes8_to_long(tcp_relay->raw_buf1 + 4);
            //log_info("req_id %lld", req_id);
            char *dubbo_package = malloc(sizeof(char) * 4096);
            //log_info("length %d", package_length);
            int dubbo_package_size = generate_res_in_place(dubbo_package, tcp_relay->raw_buf1 + 12,
                                                           package_length, req_id);

            uv_write_t *write_req = malloc(sizeof(uv_write_t));
            write_req->data = dubbo_package;
            uv_buf_t local_buf = uv_buf_init(dubbo_package, dubbo_package_size);
            uv_write(write_req, (uv_stream_t *) tcp_relay->tcp_conn_2, &local_buf, 1, free_write_req_with_data);

            tcp_relay->raw_buf1_size -= last_length;
            memmove(tcp_relay->raw_buf1, tcp_relay->raw_buf1 + last_length, tcp_relay->raw_buf1_size);
        }
    } else if (nread < 0) {
        if (nread != UV_EOF) {
            log_error("Read error %s\n", uv_strerror(nread));
        }
        //read from consumer agent, if fail or close, then we need to close the connection1
        uv_close((uv_handle_t *) tcp_relay->tcp_conn_1, p_close_tcp_relay_connection1);
    }
}


void p_on_duboo_connect(uv_connect_t *connection, int status) {
    tcp_relay_t *tcp_relay = connection->data;
    if (status == 0) {
        log_info("dubbo connect successful");

        uv_stream_t *dubbo_stream = (uv_stream_t *) tcp_relay->tcp_conn_2;
        uv_read_start(dubbo_stream, p_alloc_buffer2, p_on_read_results);//start read from dubbo
//        if (provider_scale == '0') {
//            uv_stream_t *consumer_agent = (uv_stream_t *) tcp_relay->tcp_conn_1;
//            uv_read_start(consumer_agent, p_alloc_buffer_new, p_read_connection_data_dubbo);
//        } else {
        uv_stream_t *consumer_agent = (uv_stream_t *) tcp_relay->tcp_conn_1;
        uv_read_start(consumer_agent, p_alloc_buffer1, p_read_connection_data_http_req);
        //}
    } else {
        log_info("dubbo connect failed");
        sleep(3);//sleep 3s
        log_info("retry to connection dubbo");
        connect_to_dubbo_long_live(tcp_relay);
    }

}

void connect_to_dubbo_long_live(tcp_relay_t *tcp_relay) {
    log_info("init connection to dubbo");
    p_tcp_server_t *p_tcp_server = tcp_relay->p_tcp_server;
    uv_tcp_t *dubbo_socket = (uv_tcp_t *) malloc(sizeof(uv_tcp_t));
    uv_tcp_init(p_tcp_server->work_event_loop, dubbo_socket);
    uv_tcp_nodelay(dubbo_socket, 1);
    if (tcp_relay->tcp_conn_2 != NULL) {
        free(tcp_relay->tcp_conn_2);
    }
    tcp_relay->tcp_conn_2 = dubbo_socket;
    dubbo_socket->data = tcp_relay;

    log_info("init dubbo_connection");
    uv_connect_t *dubbo_connection = malloc(sizeof(uv_connect_t));

    if (tcp_relay->dubbo_connection != NULL) {
        free(tcp_relay->dubbo_connection);
    }
    tcp_relay->dubbo_connection = dubbo_connection;
    dubbo_connection->data = tcp_relay;
    //uv_tcp_keepalive(dubbo_socket, 1, 60);
    log_info("make dubbo_connection");
    uv_tcp_connect(dubbo_connection, dubbo_socket, (const struct sockaddr *) &dubbo_addr, p_on_duboo_connect);
}

void p_on_new_connection(uv_stream_t *server, int status) {
    p_tcp_server_t *p_tcp_server = server->data;

    if (status != 0) {
        log_error("New connection error %s\n", uv_strerror(status));
        return;
    }
    uv_tcp_t *client = (uv_tcp_t *) malloc(sizeof(uv_tcp_t));
    uv_tcp_init(p_tcp_server->work_event_loop, client);
    uv_tcp_nodelay(client, 1);
    if (uv_accept(server, (uv_stream_t *) client) == 0) {
        log_info("tcp server %d accepted a connection", p_tcp_server->tid);
        tcp_relay_t *tcp_relay = init_tcp_relay_t(P_DEFAULT_PACKAGE_SIZE);
        tcp_relay->tcp_conn_1 = client;
        client->data = tcp_relay;
        tcp_relay->p_tcp_server = p_tcp_server;
        connect_to_dubbo_long_live(tcp_relay);

    } else {
        uv_close((uv_handle_t *) client, NULL);
    }
}

void p_start_listen(p_tcp_server_t *p_tcp_server) {
    log_info("start listen on %d port", server_port);
    uv_tcp_init_ex(p_tcp_server->work_event_loop, &(p_tcp_server->agent_server), AF_INET);
    uv_os_fd_t fd;
    uv_fileno((uv_handle_t *) &(p_tcp_server->agent_server), &fd);
    int optval = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    uv_tcp_nodelay(&(p_tcp_server->agent_server), 1);
    uv_tcp_bind(&(p_tcp_server->agent_server), (const struct sockaddr *) &addr, 0);
    p_tcp_server->agent_server.data = p_tcp_server;

    log_info("agent server bind successful");
    int r = uv_listen((uv_stream_t *) &(p_tcp_server->agent_server), P_DEFAULT_BACKLOG, p_on_new_connection);
    if (r) {
        log_fatal("Listen error %s", uv_strerror(r));
        exit(-1);
    }
}

void p_on_duboo_connect_test_cb(uv_connect_t *connection, int status) {
    if (status == 0) {
        log_info("dubbo connect successful");
        uv_close((uv_handle_t *) connection->handle, NULL);
    } else {
        log_info("dubbo connect failed");
        sleep(3);//sleep 3s
        log_info("retry to connection dubbo");
        test_dubbo_connection();
    }
    free(connection->handle);
    free(connection);
}

void test_dubbo_connection() {
    log_info("wait for dubbo connection...");
    uv_connect_t *dubbo_connection = malloc(sizeof(uv_connect_t));
    uv_tcp_t *dubbo_socket = (uv_tcp_t *) malloc(sizeof(uv_tcp_t));
    uv_tcp_init(main_loop, dubbo_socket);
    uv_tcp_nodelay(dubbo_socket, 1);
    log_info("make dubbo_connection");
    uv_tcp_connect(dubbo_connection, dubbo_socket, (const struct sockaddr *) &dubbo_addr, p_on_duboo_connect_test_cb);
}

void p_start_work_event_loop(void *args) {
    p_tcp_server_t *p_tcp_server = (p_tcp_server_t *) args;
    p_start_listen(p_tcp_server);

    uv_run(p_tcp_server->work_event_loop, UV_RUN_DEFAULT);
}

int provider(char *provider_type, int server_port, int dubbo_port, char *etcd_url) {
    log_info("provider_type: %s, server port: %d, dubbo port: %d, etcd_url: %s", provider_type, server_port, dubbo_port,
             etcd_url);
    main_loop = uv_default_loop();
    get_cpu_info();
    init_provider(provider_type, server_port, dubbo_port, etcd_url);
    uv_run(main_loop, UV_RUN_DEFAULT);
    log_info("start run workers");
    int P_EVENT_LOOP_N = P_EVENT_LOOP_NUM;

    log_info("use %d event loops", P_EVENT_LOOP_N);

    for (int i = 0; i < P_EVENT_LOOP_N; i++) {
        p_tcp_servers[i] = init_p_tcp_server_t(i);
    }
    uv_thread_t work_tid[P_EVENT_LOOP_N];

    for (int i = 0; i < P_EVENT_LOOP_N; i++) {
        uv_thread_create(&work_tid[i], p_start_work_event_loop, p_tcp_servers[i]);
    }

    //if main loop finish, wait for other event loops
    for (int i = 0; i < P_EVENT_LOOP_N; i++) {
        uv_thread_join(&work_tid[i]);
    }

    return 0;
}