#include "../src/conv_basic.h"

#include <assert.h>
#include <string.h>

typedef struct
{
    size_t sz_input;
    const char8_t *input;
    size_t sz_expected;
    const char32_t *expected;
} test_pair_t;

#define ADD_TEST_PAIR(str)                                                                                             \
    (test_pair_t)                                                                                                      \
    {                                                                                                                  \
        .sz_input = sizeof(u8## #str) - 1, .input = u8## #str,                                                         \
        .sz_expected = (sizeof(U## #str) / sizeof(char32_t)) - 1, .expected = U## #str                                 \
    }

int main(void)
{
    constexpr test_pair_t test_pairs[] = {
        ADD_TEST_PAIR(hello),        ADD_TEST_PAIR(world),  ADD_TEST_PAIR(ケツを食べる),
        ADD_TEST_PAIR(uoooh 😭😭😭), ADD_TEST_PAIR(🇧🇷🇧🇷🇧🇷),
    };
    constexpr size_t num_test_pairs = sizeof(test_pairs) / sizeof(test_pair_t);

    // Check the conversion is correct
    char32_t out[1024] = {0};
    for (unsigned i = 0; i < num_test_pairs; ++i)
    {
        size_t consumed, written;
        cutf_state_t ctx = {0};
        auto const res = cutf_s8tos32(test_pairs[i].sz_input, test_pairs[i].input, sizeof(out) / sizeof(*out),
                                      &consumed, out, &written, &ctx);
        assert(res == CUTF_SUCCESS);
        assert(consumed == test_pairs[i].sz_input);
        assert(written == test_pairs[i].sz_expected);
        assert(memcmp(out, test_pairs[i].expected, test_pairs[i].sz_expected * sizeof(char32_t)) == 0);
    }

    // Check that incorrect codepoints are rejected
    {
        size_t consumed, written;
        cutf_state_t ctx = {0};
        auto res = cutf_s8tos32(1, u8"\xBF", sizeof(out) / sizeof(*out), &consumed, out, &written, &ctx);
        assert(res == CUTF_INVALID_INPUT);
        res = cutf_s8tos32(1, u8"\xFF", sizeof(out) / sizeof(*out), &consumed, out, &written, &ctx);
        assert(res == CUTF_INVALID_INPUT);
    }

    return 0;
}
