#include "test_common.h"
#include <string.h>

int main(void)
{
    // Check that we get the correct length
    for (unsigned i = 0; i < num_test_pairs; ++i)
    {
        auto const len = cutf_count_s8asc32_complete(test_pairs[i].sz8, test_pairs[i].p8);
        TEST_ASSERT(len == test_pairs[i].sz32);
    }

    // Check the conversion is correct
    char32_t out[1024] = {0};
    for (unsigned i = 0; i < num_test_pairs; ++i)
    {
        size_t consumed, written;
        cutf_state_t ctx = {0};
        auto const res = cutf_s8tos32(test_pairs[i].sz8, test_pairs[i].p8, sizeof(out) / sizeof(*out),
                                      &consumed, out, &written, &ctx);
        TEST_ASSERT(res == CUTF_SUCCESS);
        TEST_ASSERT(consumed == test_pairs[i].sz8);
        TEST_ASSERT(written == test_pairs[i].sz32);
        TEST_ASSERT(memcmp(out, test_pairs[i].p32, test_pairs[i].sz32 * sizeof(char32_t)) == 0);
    }

    // Check that incorrect codepoints are rejected
    {
        size_t consumed, written;
        cutf_state_t ctx = {0};
        auto res = cutf_s8tos32(1, u8"\xBF", sizeof(out) / sizeof(*out), &consumed, out, &written, &ctx);
        TEST_ASSERT(res == CUTF_INVALID_INPUT);
        res = cutf_s8tos32(1, u8"\xFF", sizeof(out) / sizeof(*out), &consumed, out, &written, &ctx);
        TEST_ASSERT(res == CUTF_INVALID_INPUT);
    }

    return 0;
}
