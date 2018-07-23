
#include "../etcd.h"
#include "../log.h"
#include "../utils.h"
#include "../../../3rd-deps/base64/b64.h"
#include <unistd.h>

int main(int argc, char *argv[]) {

    char *id = etcd_grant(argv[1], 600);
    log_info("ID: %s", id);
    etcd_put(argv[1], "/dubbomesh/com.alibaba.dubbo.performance.demo.provider.IHelloService/192.168.1.2:8200", id);
    for (int i = 0; i < 3; i++) {
        sleep(4);//sleep 4s
        etcd_keep_alive(argv[1], id);
    }

    char **servers;
    servers = (char**) malloc(sizeof(char*)* 100);
    int servers_size = 0;
    etcd_get(argv[1], "/dubbomesh/com.alibaba.dubbo.performance.demo.provider.IHelloService/", servers, &servers_size);

    for (int i = 0; i < servers_size; i++) {
        log_info("get key: %s", servers[i]);
    }

    return 0;
}