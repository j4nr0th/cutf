#include "../src/conv_basic.h"

#include "test_common.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    // Check the conversion is correct
    char8_t out[1024] = {0};
    for (unsigned i = 0; i < num_test_pairs; ++i)
    {
        size_t consumed, written;
        cutf_state_t ctx = {0};
        auto const res = cutf_s32tos8(test_pairs[i].sz32, test_pairs[i].p32, sizeof(out) / sizeof(*out),
                                      &consumed, out, &written, &ctx);
        TEST_ASSERT(res == CUTF_SUCCESS);
        TEST_ASSERT(consumed == test_pairs[i].sz32);
        TEST_ASSERT(written == test_pairs[i].sz8);
        TEST_ASSERT(memcmp(out, test_pairs[i].p8, test_pairs[i].sz8 * sizeof(char8_t)) == 0);
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
            TEST_ASSERT((i + 2 == sizeof(full_expected) && res == CUTF_SUCCESS) || (res == CUTF_INSUFFICIENT_BUFFER));
            TEST_ASSERT(consumed < 2);
            TEST_ASSERT(written == 1);
            TEST_ASSERT(c == full_expected[i]);
        }
    }

    return 0;
}
