#pragma once

//input: /dubbomesh/com.alibaba.dubbo.performance.demo.provider.IHelloService/[S,M,L]:192.168.1.2:8200
void str_to_server_uri(char *str, char *server_addr, int *port, int *scale);

char *hostIPaddr(char *eth_name);

int four_char_to_int(char *length);

void int_to_four_char(int32_t integer, char *bytes);

void long_to_8bytes(int64_t integer, void *bytes);

int64_t bytes8_to_long(void *bytes);

float unpackFloat(const void *buf);

int packFloat(void *buf, float x);

void int_to_string(char* str, int num);

void get_cpu_info();

int64_t get_wall_time();

//this is a function from mysql-server
//https://github.com/mysql/mysql-server/blob/5.7/strings/int2str.c
char *int10_to_str(long int val,char *dst,int radix);

char *resolveToIp4Str(char *hostname);

int manual_jvm_gc(int java_pid);

void save_to_binary_file(char* filename, char* buf, int size);

void increase_long_by_one(unsigned char bytes[8]);