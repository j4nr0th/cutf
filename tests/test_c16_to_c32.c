#include "test_common.h"
#include <string.h>

int main(void)
{
    // Check the conversion is correct
    char32_t out[1024] = {0};
    for (unsigned i = 0; i < num_test_pairs; ++i)
    {
        size_t consumed, written;
        cutf_state_t ctx = {0};
        auto const res = cutf_s16tos32(test_pairs[i].sz16, test_pairs[i].p16, sizeof(out) / sizeof(*out), &consumed,
                                       out, &written, &ctx);
        TEST_ASSERT(res == CUTF_SUCCESS);
        TEST_ASSERT(consumed == test_pairs[i].sz16);
        TEST_ASSERT(written == test_pairs[i].sz32);
        TEST_ASSERT(memcmp(out, test_pairs[i].p32, test_pairs[i].sz32 * sizeof(char32_t)) == 0);
    }

    return 0;
}
