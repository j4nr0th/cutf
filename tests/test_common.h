#pragma once
#include <stdio.h>
#include <stdlib.h>
#define TEST_ASSERT(x) ((x) ? (void)0 : (fprintf(stderr, "Assertion failed: %s\n", #x), exit(EXIT_FAILURE)))

typedef struct
{
    size_t sz8;
    const char8_t *p8;
    size_t sz16;
    const char16_t *p16;
    size_t sz32;
    const char32_t *p32;
} test_pair_t;

#define ADD_TEST_PAIR(str)                                                                                             \
    (test_pair_t)                                                                                                      \
    {                                                                                                                  \
        .sz8 = (sizeof(u8## #str) / sizeof(char8_t)) - 1, .p8 = u8## #str,                                             \
        .sz16 = (sizeof(u## #str) / sizeof(char16_t)) - 1, .p16 = u## #str,                                            \
        .sz32 = (sizeof(U## #str) / sizeof(char32_t)) - 1, .p32 = U## #str                                             \
    }

static constexpr test_pair_t test_pairs[] = {
    ADD_TEST_PAIR(hello),        ADD_TEST_PAIR(world),  ADD_TEST_PAIR(ケツを食べる),
    ADD_TEST_PAIR(uoooh 😭😭😭), ADD_TEST_PAIR(🇧🇷🇧🇷🇧🇷), ADD_TEST_PAIR(モビンの時間だ),
};
static constexpr size_t num_test_pairs = sizeof(test_pairs) / sizeof(test_pair_t);
