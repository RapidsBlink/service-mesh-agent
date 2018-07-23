//
// Created by yche on 5/4/18.
//
#include "log.h"
#include "tokenizer.h"

#include <x86intrin.h>

#if defined(__BMI__)

#include <x86intrin.h>

#elif defined(__SSE4_2__)

#endif

#include <stdbool.h>
#include <memory.h>

#include "uv.h"

#include "../utils/utils.h"

const char *const post_keys[] = {"interface", "method", "parameterTypesString", "parameter"};
const int post_keys_size[] = {9, 6, 20, 9};
// magic, magic, reg/2way/fastjson-serialization, status (4 bytes); request id (8 bytes)
const char header_bytes[] = {(char) 0xda, (char) 0xbb, (char) (0x80 | 0x40 | 0x06), (char) 0x00,
                             (char) 0x00, (char) 0x00, (char) 0x00, (char) 0x00,
                             (char) 0x00, (char) 0x00, (char) 0x00, (char) 0x00};

const char *const hard_code_values[] = {"\"com.alibaba.dubbo.performance.demo.provider.IHelloService\"\n",
                                        "\"hash\"\n",
                                        "\"Ljava/lang/String;\"\n",
                                        "\n{\"path\":\"com.alibaba.dubbo.performance.demo.provider.IHelloService\"}\n"};
uv_mutex_t lock;

int serial_split(const char *str, short len, short *off) {
#ifdef DEBUG
    assert(len > 0 && len < 32767);
#endif
    off[0] = -1;
    for (short i = 0, next = 1;;) {
        while (str[i] != '=') { i++; }
        off[next] = i;
        next++;

        while (i < len && str[i] != '&') { i++; }
        off[next] = i;
        next++;

        if (i == len) { return next; }
    }
}

#if defined(__SSE4_2__)

inline int __tzcnt_u32_using_popcnt_cmpgt(unsigned int x) {
    int ret_cnt = 0;
    int half_bits = x & (0xff);
    if (half_bits == 0) {
        half_bits = (x >> 8) & (0xff);
        ret_cnt = 8;
    }

    __m128i pivot_u = _mm_set1_epi16(half_bits);
    __m128i inspected_ele = _mm_set_epi16(0xff, 0x7f, 0x3f, 0x1f, 0xf, 0x7, 0x3, 0x1);
    __m128i trunc_pivot_u = _mm_and_si128(pivot_u, inspected_ele);

    __m128i pivot_new = _mm_set1_epi16(0);
    __m128i cmp_res = _mm_cmpeq_epi16(trunc_pivot_u, pivot_new);

    int mask = _mm_movemask_epi8(cmp_res); // 16 bits
    ret_cnt += _mm_popcnt_u32(mask) >> 1;
    return ret_cnt;
}

inline int sse4_split_efficient(char *str, short len, short *off) {
#ifdef DEBUG
    assert(len > 0 && len + 16 < 32767);
#endif
    char restore_buf[16];
    memcpy(restore_buf, str + len, sizeof(char) * 16);
    memset(str + len, 0, 16);
    off[0] = -1;
    for (short i = 0, next = 1;;) {
        // 1st: advance fo find first '='
        while (true) {
            __m128i pivot_u = _mm_set1_epi8('=');
            __m128i inspected_ele = _mm_loadu_si128((__m128i *) (str + i));
            __m128i cmp_res = _mm_cmpeq_epi8(pivot_u, inspected_ele);
            int mask = _mm_movemask_epi8(cmp_res); // 16 bits

            int advance = (mask == 0 ? 16 : __tzcnt_u32_using_popcnt_cmpgt(mask));
            i += advance;
            if (advance < 16) { break; }
        }
        off[next] = i;
        next++;

        // 2nd: advance to find first '&'
        while (true) {
            __m128i pivot_u = _mm_set1_epi8('&');
            __m128i inspected_ele = _mm_loadu_si128((__m128i *) (str + i));
            __m128i cmp_res = _mm_cmpeq_epi8(pivot_u, inspected_ele);

            int mask = _mm_movemask_epi8(cmp_res); // 16 bits

            int advance = (mask == 0 ? 16 : __tzcnt_u32_using_popcnt_cmpgt(mask));
            i += advance;
            if (advance < 16 || i >= len) { break; }
        }
        off[next] = i < len ? i : len;
        next++;

        if (i >= len) {
            memcpy(str + len, restore_buf, sizeof(char) * 16);
            return next;
        }
    }
}

#endif

#if defined(__BMI__)

inline int avx_split_efficient(char *str, short len, short *off) {
#ifdef DEBUG
    assert(len > 0 && len + 16 < 32767);
#endif
    memset(str + len, 0, 16);
    off[0] = -1;
    for (short i = 0, next = 1;;) {
        // 1st: advance fo find first '='
        while (true) {
            __m128i pivot_u = _mm_set1_epi8('=');
            __m128i inspected_ele = _mm_loadu_si128((__m128i *) (str + i));
            __m128i cmp_res = _mm_cmpeq_epi8(pivot_u, inspected_ele);
            int mask = _mm_movemask_epi8(cmp_res); // 16 bits

            int advance = (mask == 0 ? 16 : __tzcnt_u32(mask));
            i += advance;
            if (advance < 16) { break; }
        }
        off[next] = i;
        next++;

        // 2nd: advance to find first '&'
        while (true) {
            __m128i pivot_u = _mm_set1_epi8('&');
            __m128i inspected_ele = _mm_loadu_si128((__m128i *) (str + i));
            __m128i cmp_res = _mm_cmpeq_epi8(pivot_u, inspected_ele);

            int mask = _mm_movemask_epi8(cmp_res); // 16 bits

            int advance = (mask == 0 ? 16 : __tzcnt_u32(mask));
            i += advance;
            if (advance < 16 || i >= len) { break; }
        }
        off[next] = i < len ? i : len;
        next++;

        if (i >= len) { return next; }
    }
}

#endif

int simd_split_efficient(char *str, short len, short *off) {
#if defined(__BMI__)
    //    printf("bmi");
    return avx_split_efficient(str, len, off);
#elif defined(__SSE4_2__)
    //    printf("sse");
    return sse4_split_efficient(str, len, off);
#else
    return serial_split(str, len, off);
#endif
}

inline int hex2int(char ch) {
    return ch < 'A' ? ch - '0' : 10 + (ch < 'a' ? ch - 'A' : ch - 'a');
}

int serialize_str(char *str, const short *off, short len_off, int offset, char *res, int j) {
    for (int i = 0; i < len_off / 2; i++) {
        int real_i = i * 2;
        int key_off_beg = off[real_i] + 1;
        int key_off_end = off[real_i + 1];
        //log_debug("key length: %d, key: %.*s", key_off_end - key_off_beg, key_off_end - key_off_beg, str+key_off_beg);
        // add post_keys[j][0] == str[key_off_beg] to reduce strncmp invocations
        if (post_keys[j][0] == str[key_off_beg] && key_off_end - key_off_beg == post_keys_size[j]) {
            if (strncmp(post_keys[j], str + key_off_beg, key_off_end - key_off_beg) == 0) {
                res[offset] = '"';
                offset++;

                // copy and decoding url e.g. '%2F' to '/', wrap as a inline function later
                int val_off_beg = off[real_i + 1] + 1;
                int val_off_end = off[real_i + 2];
                for (int last_seek, seek = val_off_beg; seek < val_off_end;) {
                    last_seek = seek;

                    // find first % or end
#if defined(__BMI__)
                    // printf("bmi...");
                    while (true) {
                        __m128i pivot_u = _mm_set1_epi8('%');
                        __m128i inspected_ele = _mm_loadu_si128((__m128i *) (str + seek));
                        __m128i cmp_res = _mm_cmpeq_epi8(pivot_u, inspected_ele);

                        int mask = _mm_movemask_epi8(cmp_res); // 16 bits

                        int advance = (mask == 0 ? 16 : __tzcnt_u32(mask));
                        seek += advance;
                        if (advance < 16 || seek >= val_off_end) { break; }
                    }
                    if (seek > val_off_end) { seek = val_off_end; }

#elif defined(__SSE4_2__)
                    // printf("sse...");
                    while (true) {
                        __m128i pivot_u = _mm_set1_epi8('%');
                        __m128i inspected_ele = _mm_loadu_si128((__m128i *) (str + seek));
                        __m128i cmp_res = _mm_cmpeq_epi8(pivot_u, inspected_ele);

                        int mask = _mm_movemask_epi8(cmp_res); // 16 bits

                        int advance = (mask == 0 ? 16 : __tzcnt_u32_using_popcnt_cmpgt(mask));
                        seek += advance;
                        if (advance < 16 || seek >= val_off_end) { break; }
                    }
                    if (seek > val_off_end) { seek = val_off_end; }
#else
                    // printf("serial...");
                    while (seek < val_off_end && str[seek] != '%') { seek++; }
#endif
                    int advance = seek - last_seek;
                    memcpy(res + offset, str + last_seek, advance);
                    offset += advance;
                    // decode %xx
                    if (seek != val_off_end) {
                        res[offset] = (char) (hex2int(str[seek + 1]) * 16 + hex2int(str[seek + 2]));
                        offset++;
                        seek += 3;  //skip "%xx"
                    }
                }

                memcpy(res + offset, "\"\n", 2);
                offset += 2;
                break;
            }
        }
    }
    return offset;
}

int serialize_path_dict(char *str, const short *off, short len_off, int offset, char *res) {
    for (int i = 0; i < len_off / 2; i++) {
        int real_i = i * 2;
        int beg = off[real_i] + 1;
        int end = off[real_i + 1];
        if (end - beg == post_keys_size[0]) {
            if (strncmp(post_keys[0], str + beg, end - beg) == 0) {
                memcpy(res + offset, "{\"path\":\"", 9);
                offset += 9;

                int advance = off[real_i + 2] - off[real_i + 1] - 1;
                memcpy(res + offset, str + off[real_i + 1] + 1, advance);

                offset += advance;
                memcpy(res + offset, "\"}\n", 3);
                offset += 3;
                break;
            }
        }
    }
    return offset;
}

void generate_header(char *res, int body_len, int64_t request_id) {
    memcpy(res, header_bytes, 12);
//    int64_t request_id = get_and_incre_req_id();
    long_to_8bytes(request_id, res + 4);

    res[12] = (char) ((body_len >> 24) & 0xFF);
    res[13] = (char) ((body_len >> 16) & 0xFF);
    res[14] = (char) ((body_len >> 8) & 0xFF);
    res[15] = (char) (body_len & 0xFF);
}

int generate_body(char *res, char *str_buffer, short *off, short len_off) {
    int offset = 16;
    memcpy(res + offset, "\"2.0.1\"\n", 8);
    offset += 8;
    offset = serialize_str(str_buffer, off, len_off, offset, res, 0);
    memcpy(res + offset, "null\n", 5);
    offset += 5;
    offset = serialize_str(str_buffer, off, len_off, offset, res, 1);
    offset = serialize_str(str_buffer, off, len_off, offset, res, 2);
    offset = serialize_str(str_buffer, off, len_off, offset, res, 3);
    offset = serialize_path_dict(str_buffer, off, len_off, offset, res);

    return offset;
}

// assume res allocated, return size
int generate_res_in_place(char *res, char *str, short len, int64_t req_id) {
    short off[32];  // 32 for at most 16 (key, val)
    short len_off = (short) simd_split_efficient(str, len, off);

    int offset = generate_body(res, str, off, len_off);
    generate_header(res, offset - 16, req_id);
    return offset;
}

static const char *method_detail_values_onea = "\"2.0.1\"\n\"com.alibaba.dubbo.performance.demo.provider.IHelloService\"\nnull\n\"hash\"\n\"Ljava/lang/String;\"\n";

int generate_body_hard_code(char *res, char *parameter_str, int parameter_len) {

    memcpy(res + 16, method_detail_values_onea, 101 * sizeof(char));
    int offset = 16 + 101;

    res[offset] = '\"';
    offset++;
    memcpy(res + offset, parameter_str, parameter_len);
    offset += parameter_len;
    res[offset] = '\"';
    offset++;

    const size_t path_len = strlen(hard_code_values[3]);
    memcpy(res + offset, hard_code_values[3], path_len);
    offset += path_len;
    return offset;
}

int generate_res_in_place_hard_code(char *res, char *str, short len, int64_t req_id) {
    int offset = generate_body_hard_code(res, str + 136, len - 136);
    generate_header(res, offset - 16, req_id);
    return offset;
}