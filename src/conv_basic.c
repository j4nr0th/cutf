#include "conv_basic.h"

#include <assert.h>

// TODO: make internal function pure to allow easier testing

enum
{
    UTF8_PREFIX_CONTINUATION = 0x80,
    UTF8_PREFIX_TWO_UNITS = 0xC0,
    UTF8_PREFIX_THREE_UNITS = 0xE0,
    UTF8_PREFIX_FOUR_UNITS = 0xF0,
};

enum
{
    MASK_BOTTOM_6_BITS = 0x3F,
    MASK_BOTTOM_12_BITS = 0xFFF,
    MASK_BOTTOM_18_BITS = 0x3FFFFF,
};

static cutf_state_t utf8_extract_leading_byte(const char8_t c8)
{
    // We are dealing with a single unit.
    if (c8 < UTF8_PREFIX_CONTINUATION)
    {
        return (cutf_state_t){.state_type = CUTF_STATE_CLEAR, .value = c8};
    }
    // Was it a surrogate codepoint?
    if (c8 < UTF8_PREFIX_TWO_UNITS)
    {
        return (cutf_state_t){.state_type = CUTF_STATE_ERROR};
    }
    // We are dealing with 2 units.
    if (c8 < UTF8_PREFIX_THREE_UNITS)
    {
        return (cutf_state_t){.state_type = CUTF_STATE_U8_1, .value = c8 & 0x1F};
    }
    // We are dealing with 3 units
    if (c8 < UTF8_PREFIX_FOUR_UNITS)
    {
        return (cutf_state_t){.state_type = CUTF_STATE_U8_2, .value = (c8 & 0x0F)};
    }
    // we are dealing with 4 units
    if (c8 < 0xF8)
    {
        return (cutf_state_t){.state_type = CUTF_STATE_U8_3, .value = (c8 & 0x07)};
    }
    // Invalid
    return (cutf_state_t){.state_type = CUTF_STATE_ERROR};
}

static cutf_state_t update_utf8_state_adding(const char8_t c, const cutf_state_t state)
{
    assert(state.state_type == CUTF_STATE_CLEAR || state.state_type == CUTF_STATE_U8_1 ||
           state.state_type == CUTF_STATE_U8_2 || state.state_type == CUTF_STATE_U8_3);

    if (state.state_type == CUTF_STATE_CLEAR)
    {
        // We have a new one to deal with
        return utf8_extract_leading_byte(c);
    }

    // Check that this is indeed a surrogate unit
    if (c < UTF8_PREFIX_CONTINUATION || c > 0xBF)
        return (cutf_state_t){.state_type = CUTF_STATE_ERROR};

    cutf_state_type_t new_type;

    switch (state.state_type)
    {
    case CUTF_STATE_U8_3:
        new_type = CUTF_STATE_U8_2;
        break;

    case CUTF_STATE_U8_2:
        new_type = CUTF_STATE_U8_1;
        break;

    case CUTF_STATE_U8_1:
        new_type = CUTF_STATE_CLEAR;
        break;

    default:
        return (cutf_state_t){.state_type = CUTF_STATE_ERROR};
    }

    return (cutf_state_t){.state_type = new_type, .value = (state.value << 6) | (c & MASK_BOTTOM_6_BITS)};
}

cutf_result_t cutf_c8toc32(const size_t sz_in, const char8_t p_in[const static sz_in], size_t *const p_consumed,
                           char32_t *const p_out, cutf_state_t *const ctx)
{
    cutf_result_t res = CUTF_INCOMPLETE_INPUT;
    size_t i;
    // Keep going until we either run out of characters or conversion is no longer incomplete.
    for (i = 0; i < sz_in; ++i)
    {
        auto const c = p_in[i];

        // Update the state type
        auto const new_state = update_utf8_state_adding(c, *ctx);
        if (new_state.state_type == CUTF_STATE_ERROR)
            return CUTF_INVALID_INPUT;

        // Update the value accumulated thus far
        *ctx = new_state;
        if (new_state.state_type == CUTF_STATE_CLEAR)
        {
            // We are done with parsing
            *p_out = new_state.value;
            // Clear the context value as well
            ctx->value = 0;
            res = CUTF_SUCCESS;
            i += 1;
            break;
        }
    }

    *p_consumed = i;
    return res;
}

cutf_result_t cutf_s8tos32(const size_t sz_in, const char8_t p_in[const static sz_in], const size_t sz_out,
                           size_t *const p_consumed, char32_t p_out[const sz_out], size_t *const p_written,
                           cutf_state_t *const ctx)
{
    size_t pos_in = 0, pos_out = 0;
    while (pos_in < sz_in && pos_out < sz_out)
    {
        size_t consumed;
        auto const res = cutf_c8toc32(sz_in - pos_in, p_in + pos_in, &consumed, p_out + pos_out, ctx);
        if (res != CUTF_SUCCESS)
        {
            return res;
        }
        pos_in += consumed;
        pos_out += 1;
    }
    *p_consumed = pos_in;
    *p_written = pos_out;

    // Check if we ran out of output buffer before the end of the input
    if (pos_out == sz_out)
        return CUTF_INSUFFICIENT_BUFFER;

    // Nope, we finished it all!
    return CUTF_SUCCESS;
}

typedef struct
{
    cutf_state_t state;
    char8_t out;
} remove_result_t;

static remove_result_t update_utf8_state_removing(const cutf_state_t state, const char32_t c)
{
    assert(state.state_type == CUTF_STATE_CLEAR || state.state_type == CUTF_STATE_U8_1 ||
           state.state_type == CUTF_STATE_U8_2 || state.state_type == CUTF_STATE_U8_3);

    if (state.state_type == CUTF_STATE_CLEAR)
    {
        // Initialize the state
        // Will it be a single unit?
        if (c < UTF8_PREFIX_CONTINUATION)
        {
            return (remove_result_t){.state = {.state_type = CUTF_STATE_CLEAR, .value = 0}, .out = c};
        }
        // Will it be two units?
        if (c < 0x800)
        {
            // Split into the leading part and the remainder
            const char8_t leading = (char8_t)UTF8_PREFIX_TWO_UNITS | (char8_t)(c >> 6); // Leading
            auto const remainder = c & MASK_BOTTOM_6_BITS;                              // Remainder 6 bits
            return (remove_result_t){.state = {.state_type = CUTF_STATE_U8_1, .value = remainder}, .out = leading};
        }
        // Will it be three units?
        if (c < 0x10000)
        {
            // Split into the leading part and the remainder
            const char8_t leading = (char8_t)UTF8_PREFIX_THREE_UNITS | (char8_t)(c >> 12); // Leading
            auto const remainder = c & MASK_BOTTOM_12_BITS;                                // Remainder 12 bits
            return (remove_result_t){.state = {.state_type = CUTF_STATE_U8_2, .value = remainder}, .out = leading};
        }
        // Will it be four units?
        if (c < 0x110000)
        {
            // Split into the leading part and the remainder
            const char8_t leading = (char8_t)UTF8_PREFIX_FOUR_UNITS | (char8_t)(c >> 18); // Leading
            auto const remainder = c & MASK_BOTTOM_18_BITS;                               // Remaining 18 bits
            return (remove_result_t){.state = {.state_type = CUTF_STATE_U8_3, .value = remainder}, .out = leading};
        }

        // Invalid
        return (remove_result_t){.state = {.state_type = CUTF_STATE_ERROR}};
    }

    // The current state is not clean, which means we extract the next top 6 bits.
    // This also means we do not need "c"

    cutf_state_type_t new_type;
    char8_t out;
    char32_t v;

    switch (state.state_type)
    {
    case CUTF_STATE_U8_3:
        // 18 bits left, so take the top 6
        out = (char8_t)UTF8_PREFIX_CONTINUATION | (char8_t)(state.value >> 12);
        v = state.value & MASK_BOTTOM_12_BITS;
        new_type = CUTF_STATE_U8_2;
        break;

    case CUTF_STATE_U8_2:
        // 12 bits left, so take the top 6
        out = (char8_t)UTF8_PREFIX_CONTINUATION | (char8_t)(state.value >> 6);
        v = state.value & MASK_BOTTOM_6_BITS;
        new_type = CUTF_STATE_U8_1;
        break;

    case CUTF_STATE_U8_1:
        // 6 bits left, take all and clear the state
        out = (char8_t)UTF8_PREFIX_CONTINUATION | (char8_t)(state.value);
        v = 0;
        new_type = CUTF_STATE_CLEAR;
        break;

    default:
        return (remove_result_t){.state = {.state_type = CUTF_STATE_ERROR}};
    }

    return (remove_result_t){.state = {.state_type = new_type, .value = v}, .out = out};
}

cutf_result_t cutf_s32tos8(const size_t sz_in, const char32_t p_in[const static sz_in], const size_t sz_out,
                           size_t *const p_consumed, char8_t p_out[const sz_out], size_t *const p_written,
                           cutf_state_t *const ctx)
{
    size_t pos_out, pos_in;
    for (pos_out = 0, pos_in = 0; pos_out < sz_out; ++pos_out)
    {
        // Advance the state
        remove_result_t res;
        if (ctx->state_type != CUTF_STATE_CLEAR)
        {
            // We do not need to read any more characters at the moment
            res = update_utf8_state_removing(*ctx, 0);
        }
        else
        {
            // We need to read the next codepoint
            if (pos_in >= sz_in)
            {
                break;
            }
            auto const c = p_in[pos_in];
            pos_in += 1;
            res = update_utf8_state_removing(*ctx, c);
        }

        // Was the state possible to advance?
        if (res.state.state_type == CUTF_STATE_ERROR)
        {
            return CUTF_INVALID_INPUT;
        }

        // Update the state with the new state and write out the next byte
        *ctx = res.state;
        p_out[pos_out] = res.out;
    }
    *p_consumed = pos_in;
    *p_written = pos_out;

    // Check if we ran out of output buffer before the end of the input
    if (pos_out == sz_out)
        return CUTF_INSUFFICIENT_BUFFER;

    // Nope, we finished it all!
    return CUTF_SUCCESS;
}
