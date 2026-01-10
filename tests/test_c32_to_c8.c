#include "../src/conv_basic.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct
{
    size_t sz_input;
    const char32_t *input;
    size_t sz_expected;
    const char8_t *expected;
} test_pair_t;

#define ADD_TEST_PAIR(str)                                                                                             \
    (test_pair_t)                                                                                                      \
    {                                                                                                                  \
        .sz_input = sizeof(U## #str) / sizeof(char32_t) - 1, .input = U## #str,                                        \
        .sz_expected = (sizeof(u8## #str) / sizeof(char8_t)) - 1, .expected = u8## #str                                \
    }

int main(void)
{
    constexpr test_pair_t test_pairs[] = {
        ADD_TEST_PAIR(hello),        ADD_TEST_PAIR(world),  ADD_TEST_PAIR(ケツを食べる),
        ADD_TEST_PAIR(uoooh 😭😭😭), ADD_TEST_PAIR(🇧🇷🇧🇷🇧🇷),
    };
    constexpr size_t num_test_pairs = sizeof(test_pairs) / sizeof(test_pair_t);

    // Check the conversion is correct
    char8_t out[1024] = {0};
    for (unsigned i = 0; i < num_test_pairs; ++i)
    {
        size_t consumed, written;
        cutf_state_t ctx = {0};
        auto const res = cutf_s32tos8(test_pairs[i].sz_input, test_pairs[i].input, sizeof(out) / sizeof(*out),
                                      &consumed, out, &written, &ctx);
        assert(res == CUTF_SUCCESS);
        assert(consumed == test_pairs[i].sz_input);
        assert(written == test_pairs[i].sz_expected);
        assert(memcmp(out, test_pairs[i].expected, test_pairs[i].sz_expected * sizeof(char8_t)) == 0);
    }

    // Check that conversion works even bit by bit
    {
        constexpr char8_t full_expected[] = u8"ケツを食べる";
        const char32_t *const full_input = U"ケツを食べる";
        cutf_state_t state = {0};
        size_t consumed, written;
        // Go one byte at a time and keep the state the same
        for (unsigned i = 0, j = 0; i + 1 < sizeof(full_expected); ++i)
        {
            char8_t c;
            auto const res = cutf_s32tos8(1, full_input + j, 1, &consumed, &c, &written, &state);
            j += consumed;
            assert((i + 2 == sizeof(full_expected) && res == CUTF_SUCCESS) || (res == CUTF_INSUFFICIENT_BUFFER));
            assert(consumed < 2);
            assert(written == 1);
            assert(c == full_expected[i]);
        }
    }

    return 0;
}
