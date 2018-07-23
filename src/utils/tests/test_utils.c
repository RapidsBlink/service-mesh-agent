#include <unistd.h>
#include <stdint.h>
#include <assert.h>

#include "../etcd.h"
#include "../log.h"
#include "../utils.h"


int main(int argc, char *argv[]) {

    char *addr = hostIPaddr("eth0");
    if (strlen(addr) == 0)
        addr = hostIPaddr("eno1");
    log_info("%s", addr);

    char int_s[40];

    log_info("wall time:%lld", get_wall_time());
    int_to_string(int_s, 123);
    log_info("wall time:%lld", get_wall_time());

    log_info("%s", int_s);

    log_info("wall time:%lld", get_wall_time());
    usleep(1000);
    log_info("wall time:%lld", get_wall_time());
    log_info("max int64: %lld:", INT64_MAX);

    log_info("ip of www.notfound.me: %s", resolveToIp4Str("www.notfound.me"));

    char buf[100];
    packFloat(buf, 0.1212);
    log_info("unpack float %f", unpackFloat(buf));

    static const char *method_detail_values_one = "\n{\"path\":\"com.alibaba.dubbo.performance.demo.provider.IHelloService\"}\n";
    log_info("str length %d", strlen(method_detail_values_one));

    unsigned char integer[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    for (int i = 0; i < 1024; i++) {
        increase_long_by_one(integer);
    //    log_info("interger = %lld", *((uint64_t *) integer));
        assert(*((uint64_t *) integer) == i + 1);
    }

    for(int i = 0 ; i < 1024; i ++) {
        int length = i;
        int_to_four_char(length, buf);
        assert(four_char_to_int(buf) == length);
    }

    for(int i = 0 ; i < 1024; i ++) {
        int64_t length = i;
        long_to_8bytes(length, buf);
        assert(bytes8_to_long(buf) == length);
    }

    return 0;
}