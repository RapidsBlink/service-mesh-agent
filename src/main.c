#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "utils/log.h"
#include "consumer_agent.h"
#include "provider_agent.h"

//argv[0]->program name, argv[1]->type(consumer agent or provider agent)
//argv[2]->server port, argv[3]->dubbo.protocol.port, argv[4]->etcd_url, argv[5]->log file
int main(int argc, char *argv[]) {
    if (argc < 6) {
        log_fatal("invalid arguments, exiting.");
        return -1;
    }

    //set log file descriptor
#ifdef USE_LOG
    FILE *log_f;
    log_f = fopen(argv[5], "a+");
    log_set_fp(log_f);
#endif

#ifdef GIT_SHA1
    log_info("GIT VERSION: %s", GIT_SHA1);
#endif

    log_info("agent started, type: %s", argv[1]);

    //extract arguments
    int server_port = 0, dubbo_port = 0;
    char *etcd_url;
    server_port = atoi(argv[2]);
    dubbo_port = atoi(argv[3]);
    etcd_url = argv[4];
    log_info("arguments: server_port->%d, dubbo_port->%d, etcd_url->%s", server_port, dubbo_port, etcd_url);

    if (strcmp(argv[1], "consumer") == 0) {
        consumer(server_port, etcd_url);
    } else if (strncmp(argv[1], "provider", 8) == 0) {
        provider(argv[1], server_port, dubbo_port, etcd_url);
    } else {
        log_fatal("invalid agent type: %s", argv[1]);
    }

#ifdef USE_LOG
    fclose(log_f);
#endif
    return 0;
}