//
// Created by yche on 5/4/18.
//

#ifndef AGENT_TOKENIZER_H
#define AGENT_TOKENIZER_H

#include <stdlib.h>

int serial_split(const char *str, short len, short *off);

int simd_split_efficient(char *str, short len, short *off);

int generate_body(char *res, char *str_buffer, short *off, short len_off);

void generate_header(char *res, int body_len, int64_t req_id);

int generate_res_in_place(char *res, char *str, short len, int64_t req_id);

int generate_res_in_place_hard_code(char *res, char *str, short len, int64_t req_id);

// fast req pacakge
int generate_fast_req_package_in_place(char *res, char *str, short len, int64_t req_id);

int generate_dubbo_package(char *res, char *fast_req_package, int fast_req_len);
#endif //AGENT_TOKENIZER_H
