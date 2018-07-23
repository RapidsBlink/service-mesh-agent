//
// Created by yche on 5/16/18.
//

#include <memory.h>

#include "tokenizer.h"
#include "utils.h"

extern const char header_bytes[];

const char *const method_detail_path = "\n{\"path\":\"com.alibaba.dubbo.performance.demo.provider.IHelloService\"}\n";
static const char *method_detail_values_one = "\"2.0.1\"\n\"com.alibaba.dubbo.performance.demo.provider.IHelloService\"\nnull\n\"hash\"\n\"Ljava/lang/String;\"\n";

void generate_fast_req_package_header(char *res, int body_len, int64_t request_id) {
    // 1st: length (4 bytes)
    int_to_four_char(body_len, res);
    // 2nd: left empty (2 bytes), for serialization method and service type id
    // 3rd: req id (8 bytes)
    long_to_8bytes(request_id, res + 6);
}

// assume res allocated, return size
int generate_fast_req_package_in_place(char *res, char *str, short len, int64_t req_id) {
    int body_len = len - 136;
    //log_info("body length %d", body_len);
    memcpy(res + 14, str + 136, body_len);
    generate_fast_req_package_header(res, body_len, req_id);
    return body_len + 14;
}

int generate_dubbo_package(char *res, char *fast_req_package, int fast_req_len) {
    // 1st: magic, event, status (4bytes), req id (8 bytes)
    memcpy(res, header_bytes, 4);
    memcpy(res + 4, fast_req_package + 6, 8);

    // 2nd: body
    memcpy(res + 16, method_detail_values_one, 101 * sizeof(char));
    int offset = 16 + 101;

    res[offset] = '\"';
    offset++;
    memcpy(res + offset, fast_req_package + 14, fast_req_len - 14);
    offset += fast_req_len - 14;
    res[offset] = '\"';
    offset++;

    memcpy(res + offset, method_detail_path, 70);
    offset += 70;

    // 3rd: add the body length header
    int_to_four_char(offset - 16, res + 12);
    return offset;
}