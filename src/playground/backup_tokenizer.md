```cpp
int avx_split_baseline(char *str, short len, short *off) {
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
            mask |= 1 << 16;
            int advance = __tzcnt_u32(mask);

            i += advance;
            if (advance != 16) { break; }
        }
        off[next] = i;
        next++;

        // 2nd: advance to find first '&'
        while (true) {
            __m128i pivot_u = _mm_set1_epi8('&');
            __m128i inspected_ele = _mm_loadu_si128((__m128i *) (str + i));
            __m128i cmp_res = _mm_cmpeq_epi8(pivot_u, inspected_ele);
            int mask = _mm_movemask_epi8(cmp_res); // 16 bits
            mask |= 1 << 16;
            int advance = __tzcnt_u32(mask);

            i += advance;
            if (advance != 16 || i >= len) { break; }
        }
        off[next] = i < len ? i : len;
        next++;

        if (i >= len) { return next; }
    }
}
```