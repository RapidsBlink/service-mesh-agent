//
// Created by yche on 5/16/18.
//

#include <memory.h>
#include <assert.h>

#include "../utils/log.h"
#include "../utils/tokenizer.h"
#include "../utils/utils.h"

const char *my_str = "interface=com.alibaba.dubbo.performance.demo.provider.IHelloService&"
                     "method=hash&parameterTypesString=Ljava%2Flang%2FString%3B"
                     "&parameter=Tbk4ZGqnHQNRM8Wqr65Sxz8K2wnWHhvcaNuAnTn64geI6AnEHB8cCtEg154"
                     "rqjTqWIXqMdUwysbjwTivtkbLi8qNHWg1Kri58NPcKS3mpe1lZ0bh48dDhgoNRoAkL548Lz"
                     "GKQNiZb2cKX6geL7srwMjbZ1ybLvCNkHFnMd6cpIMMO8mvyAA3LLH3RydyTGEv2JLP6N9lp"
                     "3xE1YnI8O28TMu0XKeqBaNzpHQDrNDAMSNZn7Ka896JnEiFr1naM5h7fyff1BqXUfA5rZSed"
                     "kSXQt4RyY9WvmhEeF3eOfapdb34P64uLlJqfNcBpjfKB5sHaeUmDpBI6j18XhxqvdeE2K69dFS"
                     "dXavGri0aLyeUlCUX3VE5owNt27yeYNH76qHEC0ZJpBW9BnUTo4BuojqhnyfIkdTb7IEl8lyns"
                     "6uJPJXTSIuZloVqCcZxoqvh9dlj69BIVLaksjhttv2lAuF9raq5ceXkHWrsgfLEvwphvCc5gHAgliG";

int main() {
    // 1st: test encoding middle package
    char *res = malloc(4096);
    char *my_str_buf = malloc(4096);
    strcpy(my_str_buf, my_str);

    int package_size = generate_fast_req_package_in_place(res, my_str_buf, (short) strlen(my_str), 1234678901);
    // body len, reserved 2bytes, req id, string
    log_info("pacakge size: %d", package_size);
    log_info("packcage info: (%d, %ld, %.*s)", four_char_to_int(res + 0), bytes8_to_long(res + 4 + 2),
             four_char_to_int(res + 0), res + 4 + 2 + 8);

    // 2nd: test decoding middle package into dubbo package
    int body_size = four_char_to_int(res + 0);
    char *res_dubbo = malloc(4096);

    int dubbo_size = generate_dubbo_package(res_dubbo, res, body_size);

    log_info("dubbo header magics: 0x%x, 0x%x", res_dubbo[0] & 0xff, res_dubbo[1] & 0xff);
    log_info("dubbo header event, status: 0x%x, 0x%x", res_dubbo[2] & 0xff, res_dubbo[3] & 0xff);
    log_info("dubbo req id:", bytes8_to_long(res_dubbo + 4));
    log_info("dubbo body len:", four_char_to_int(res_dubbo + 12));
    assert(four_char_to_int(res_dubbo + 12) == dubbo_size - 16);
    log_info("dubbo body: %.*s", dubbo_size - 16, res_dubbo + 16);

    free(my_str_buf);
    free(res);
    free(res_dubbo);
}