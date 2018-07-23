#pragma once

#include <string.h>
#include <stdlib.h>

typedef struct {
    char *ptr;
    size_t len;
} res_string;

void free_string(res_string *s);

void init_string(res_string *s);

char *etcd_grant(char *etcd_url, int ttl);

int etcd_put(char *etcd_url, char *key, char *leaseID);

int etcd_get(char *etcd_url, char *key, char **server_addr, int *results_size);

int etcd_keep_alive(char *etcd_url, char *leaseID);