#pragma once

#include <stdint.h>
#include <uchar.h>

enum cutf_result_t
{
    CUTF_SUCCESS,
    CUTF_INCOMPLETE_INPUT,
    CUTF_INSUFFICIENT_BUFFER,
    CUTF_INVALID_INPUT,
};
typedef enum cutf_result_t cutf_result_t;

enum cutf_state_type_t
{
    CUTF_STATE_CLEAR, // Nothing currently in the state
    CUTF_STATE_U8_1,  // Parsing UTF8, need 1 more surrogate
    CUTF_STATE_U8_2,  // Parsing UTF8, need 2 more surrogates
    CUTF_STATE_U8_3,  // Parsing UTF8, need 3 more surrogates
    CUTF_STATE_U16_1, // Parsing UTF16, need 1 more surrogate
    CUTF_STATE_ERROR, // Error state
};
typedef enum cutf_state_type_t cutf_state_type_t;

struct cutf_state_t
{
    cutf_state_type_t state_type;
    char32_t value;
};
typedef struct cutf_state_t cutf_state_t;

cutf_result_t cutf_c8toc32(size_t sz_in, const char8_t p_in[static sz_in], size_t *p_consumed, char32_t *p_out,
                           cutf_state_t *ctx);

cutf_result_t cutf_s8tos32(size_t sz_in, const char8_t p_in[static sz_in], size_t sz_out, size_t *p_consumed,
                           char32_t p_out[sz_out], size_t *p_written, cutf_state_t *ctx);

cutf_result_t cutf_s32tos8(size_t sz_in, const char32_t p_in[const static sz_in], size_t sz_out, size_t *p_consumed,
                           char8_t p_out[const sz_out], size_t *p_written, cutf_state_t *ctx);
