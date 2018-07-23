//
// Created by yche on 5/3/18.
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <memory.h>
#include <uv.h>

#include "../utils/tokenizer.h"

//const char *my_str = "interface=com.alibaba.dubbo.performance.demo.provider.IHelloService&"
//                     "method=hash&parameterTypesString=Ljava%2Flang%2FString%3B"
//                     "&parameter=abc";
const char *my_str = "interface=com.alibaba.dubbo.performance.demo.provider.IHelloService&"
                     "method=hash&parameterTypesString=Ljava%2Flang%2FString%3B"
                     "&parameter=Tbk4ZGqnHQNRM8Wqr65Sxz8K2wnWHhvcaNuAnTn64geI6AnEHB8cCtEg154"
                     "rqjTqWIXqMdUwysbjwTivtkbLi8qNHWg1Kri58NPcKS3mpe1lZ0bh48dDhgoNRoAkL548Lz"
                     "GKQNiZb2cKX6geL7srwMjbZ1ybLvCNkHFnMd6cpIMMO8mvyAA3LLH3RydyTGEv2JLP6N9lp"
                     "3xE1YnI8O28TMu0XKeqBaNzpHQDrNDAMSNZn7Ka896JnEiFr1naM5h7fyff1BqXUfA5rZSed"
                     "kSXQt4RyY9WvmhEeF3eOfapdb34P64uLlJqfNcBpjfKB5sHaeUmDpBI6j18XhxqvdeE2K69dFS"
                     "dXavGri0aLyeUlCUX3VE5owNt27yeYNH76qHEC0ZJpBW9BnUTo4BuojqhnyfIkdTb7IEl8lyns"
                     "6uJPJXTSIuZloVqCcZxoqvh9dlj69BIVLaksjhttv2lAuF9raq5ceXkHWrsgfLEvwphvCc5gHAgliG";

long timediff(clock_t t1, clock_t t2) {
    long elapsed;
    elapsed = (long) (((double) t2 - t1) / CLOCKS_PER_SEC * 1000);
    return elapsed;
}

void test_split() {
    char *buffer = malloc(sizeof(char) * 4096);

    short *off = malloc(sizeof(short) * 16);
    short len = (short) strlen(my_str);
    memcpy(buffer, my_str, len);

    // 1st: usage demo
    int size = simd_split_efficient(buffer, len, off);
    printf("%s, \n", my_str);
    printf("=,& : %d, %d\n", '=', '&');
    printf("%d, %d\n", len, size);
    for (int i = 0; i < size; i++) { printf("%d\n", off[i]); }

    // 2nd: measure time -----------------
    // 1) avx - efficient
    clock_t t1, t2;
    long elapsed;
    t1 = clock();
    for (int i = 0; i < 5000 * 60; i++) {
        simd_split_efficient(buffer, len, off);
    }
    t2 = clock();
    elapsed = timediff(t1, t2);
    printf("simd elapsed: %ld ms\n", elapsed);

    // 2) serial
    t1 = clock();
    for (int i = 0; i < 5000 * 60; i++) {
        serial_split(buffer, len, off);
    }
    t2 = clock();
    elapsed = timediff(t1, t2);
    printf("serial elapsed: %ld ms\n", elapsed);
    // end of measuring time --------------------------

    free(off);
    free(buffer);
}

void test_generate_body() {
    printf("-------------------\n");
    short len = (short) strlen(my_str);
    char *str_buffer = malloc(sizeof(char) * 4096);
    memcpy(str_buffer, my_str, len);

    clock_t t1, t2;
    long elapsed;
    int64_t req_id = 0;
    t1 = clock();
    for (int i = 0; i < 5000 * 60; i++) {
        char *res = malloc(sizeof(char) * 2048);
        generate_res_in_place_hard_code(res, str_buffer, len, req_id);
        req_id++;
        free(res);
    }
    t2 = clock();
    elapsed = timediff(t1, t2);
    printf("total transform time: %ld ms\n", elapsed);

    // last round
    char *res = malloc(sizeof(char) * 2048);
    int offset = generate_res_in_place(res, str_buffer, len, req_id);

    printf("header:");
    for (int i = 0; i < 16; i++) { printf("0x%x ,", res[i] & 0xff); }
    printf("\ntotal length: %d\n", offset - 16);
    printf("%.*s", offset - 16, res + 16);
    printf("after the new line~~~~~~~~~");
    free(str_buffer);
    free(res);
}

int main() {
    test_split();
    test_generate_body();
}
