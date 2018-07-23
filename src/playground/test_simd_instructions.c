//
// Created by yche on 5/4/18.
//

#include <stdio.h>
#include <immintrin.h> // AVX

#include "../utils/utils.h"

void test_avx_tzcnt() {
    int x = 1 << 8 | 1 << 3;
    printf("x:%d, tz_cnt of x:%d\n", x, __tzcnt_u32(x));
    printf("x:%d, pop_cnt of x:%d\n", x, _mm_popcnt_u32(x));

    x = 0;
    printf("x:%d, tz_cnt of x:%d\n", x, __tzcnt_u32(x));
    printf("x:%d, pop_cnt of x:%d\n", x, _mm_popcnt_u32(x));
}

void test_long_8_byte_covnert() {
    char bytes[8];
    int64_t num = 1234;
    long_to_8bytes(num, bytes);
    for (int i = 0; i < 8; i++) {
        printf("0x%x,", bytes[i] & 0xff);
    }
    printf("\n%ld", bytes8_to_long(bytes));
}

void test_int_4_byte_covnert() {
    char bytes[4];
    int32_t num = 299;
    int_to_four_char(num, bytes);
    for (int i = 0; i < 4; i++) {
        printf("0x%x,", bytes[i] & 0xff);
    }
    printf("\n%d\n", four_char_to_int(bytes));
}

int main() {
    test_avx_tzcnt();
    test_int_4_byte_covnert();
    test_long_8_byte_covnert();
}