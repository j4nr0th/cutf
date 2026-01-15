#include "test_common.h"
#include <string.h>

int main(void)
{
    // Check the conversion is correct
    char8_t out[1024] = {0};
    for (unsigned i = 0; i < num_test_pairs; ++i)
    {
        size_t consumed, written;
        cutf_state_t ctx = {0};
        auto const res = cutf_s16tos8(test_pairs[i].sz16, test_pairs[i].p16, sizeof(out) / sizeof(*out), &consumed, out,
                                      &written, &ctx);
        TEST_ASSERT(res == CUTF_SUCCESS);
        TEST_ASSERT(consumed == test_pairs[i].sz16);
        TEST_ASSERT(written == test_pairs[i].sz8);
        TEST_ASSERT(memcmp(out, test_pairs[i].p8, test_pairs[i].sz8 * sizeof(char8_t)) == 0);
    }

    // Check that incorrect codepoints are rejected
    {
        size_t consumed, written;
        cutf_state_t ctx = {0};
        auto res = cutf_s16tos8(1, u"\xDDBF", sizeof(out) / sizeof(*out), &consumed, out, &written, &ctx);
        TEST_ASSERT(res == CUTF_INVALID_INPUT);
        res = cutf_s16tos8(1, u"\xDDFF", sizeof(out) / sizeof(*out), &consumed, out, &written, &ctx);
        TEST_ASSERT(res == CUTF_INVALID_INPUT);
    }

    // Check the conversion works byte by byte
    for (unsigned i = 0; i < num_test_pairs; ++i)
    {
        size_t consumed, written;
        cutf_state_t ctx = {0};
        cutf_result_t res = ~0; // Garbage
        for (unsigned p_in = 0, p_out = 0; p_out < test_pairs[i].sz8;)
        {
            res = cutf_s16tos8(1, test_pairs[i].p16 + p_in, 1, &consumed, out + p_out, &written, &ctx);
            TEST_ASSERT(res == CUTF_SUCCESS || res == CUTF_INCOMPLETE_INPUT);
            TEST_ASSERT(consumed == 1 || written == 1);
            p_out += written;
            p_in += consumed;
        }
        TEST_ASSERT(res == CUTF_SUCCESS);
        TEST_ASSERT(memcmp(out, test_pairs[i].p8, test_pairs[i].sz8 * sizeof(char8_t)) == 0);
    }

    return 0;
}
