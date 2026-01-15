#include "../include/cutf.h"

#include <assert.h>
#include <stdint.h>

typedef enum
{
    UNICODE_MAX_VALUE = 0x10FFFF,   // highest unicode value
    UNICODE_INVALID_START = 0xD800, // start of the invalid region
    UNICODE_INVALID_END = 0xDFFF,   // end of the invalid region
} unicode_limits_t;

static bool is_valid_unicode_codepoint(const char32_t c)
{
    return (c > UNICODE_MAX_VALUE || (c >= UNICODE_INVALID_START && c <= UNICODE_INVALID_END)) == 0;
}

typedef enum
{
    UTF8_PREFIX_CONTINUATION = 0x80,
    UTF8_PREFIX_TWO_UNITS = 0xC0,
    UTF8_PREFIX_THREE_UNITS = 0xE0,
    UTF8_PREFIX_FOUR_UNITS = 0xF0,
    UTF8_MAX_TWO_UNITS = 0x800,
    UTF8_MAX_THREE_UNITS = 0x10000,
    UTF8_MAX_FOUR_UNITS = 0x110000,
} utf8_constants_t;

typedef enum
{
    MASK_BOTTOM_6_BITS = 0x3F,
    MASK_BOTTOM_12_BITS = 0xFFF,
    MASK_BOTTOM_18_BITS = 0x3FFFFF,
    MASK_BOTTOM_10_BITS = 0x3FF,
} bit_masks_t;

static cutf_state_type_t utf8_classify_leading_byte(const char8_t c8)
{

    // We are dealing with a single unit.
    if (c8 < UTF8_PREFIX_CONTINUATION)
    {
        return CUTF_STATE_CLEAR;
    }
    // Was it a surrogate codepoint?
    if (c8 < UTF8_PREFIX_TWO_UNITS)
    {
        return CUTF_STATE_ERROR;
    }
    // We are dealing with 2 units.
    if (c8 < UTF8_PREFIX_THREE_UNITS)
    {
        return CUTF_STATE_U8_1;
    }
    // We are dealing with 3 units
    if (c8 < UTF8_PREFIX_FOUR_UNITS)
    {
        return CUTF_STATE_U8_2;
    }
    // we are dealing with 4 units
    if (c8 < 0xF8)
    {
        return CUTF_STATE_U8_3;
    }
    // Invalid
    return CUTF_STATE_ERROR;
}

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
                           char32_t *const p_out, cutf_state_t *const state)
{
    cutf_result_t res = CUTF_INCOMPLETE_INPUT;
    size_t i;
    // Keep going until we either run out of characters or conversion is no longer incomplete.
    for (i = 0; i < sz_in; ++i)
    {
        auto const c = p_in[i];

        // Update the state type
        auto const new_state = update_utf8_state_adding(c, *state);
        if (new_state.state_type == CUTF_STATE_ERROR)
            return CUTF_INVALID_INPUT;

        // Update the value accumulated thus far
        *state = new_state;
        if (new_state.state_type == CUTF_STATE_CLEAR)
        {
            // We are done with parsing
            *p_out = new_state.value;
            // Clear the context value as well
            state->value = 0;
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
                           cutf_state_t *const state)
{
    size_t pos_in = 0, pos_out = 0;
    while (pos_in < sz_in && pos_out < sz_out)
    {
        size_t consumed;
        auto const res = cutf_c8toc32(sz_in - pos_in, p_in + pos_in, &consumed, p_out + pos_out, state);
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
    if (pos_in != sz_in)
        return CUTF_INSUFFICIENT_BUFFER;

    // Nope, we finished it all!
    return CUTF_SUCCESS;
}

typedef struct
{
    cutf_state_t state;
    char8_t out;
} remove_result_utf8_t;

static remove_result_utf8_t update_utf8_state_removing(const cutf_state_t state, const char32_t c)
{
    assert(state.state_type == CUTF_STATE_CLEAR || state.state_type == CUTF_STATE_U8_1 ||
           state.state_type == CUTF_STATE_U8_2 || state.state_type == CUTF_STATE_U8_3);

    if (state.state_type == CUTF_STATE_CLEAR)
    {
        // Check for valid Unicode codepoint
        if (!is_valid_unicode_codepoint(c))
            return (remove_result_utf8_t){.state = {.state_type = CUTF_STATE_ERROR}};

        // Initialize the state
        // Will it be a single unit?
        if (c < UTF8_PREFIX_CONTINUATION)
        {
            return (remove_result_utf8_t){.state = {.state_type = CUTF_STATE_CLEAR, .value = 0}, .out = c};
        }
        // Will it be two units?
        if (c < UTF8_MAX_TWO_UNITS)
        {
            // Split into the leading part and the remainder
            const char8_t leading = (char8_t)UTF8_PREFIX_TWO_UNITS | (char8_t)(c >> 6); // Leading
            auto const remainder = c & MASK_BOTTOM_6_BITS;                              // Remainder 6 bits
            return (remove_result_utf8_t){.state = {.state_type = CUTF_STATE_U8_1, .value = remainder}, .out = leading};
        }
        // Will it be three units?
        if (c < UTF8_MAX_THREE_UNITS)
        {
            // Split into the leading part and the remainder
            const char8_t leading = (char8_t)UTF8_PREFIX_THREE_UNITS | (char8_t)(c >> 12); // Leading
            auto const remainder = c & MASK_BOTTOM_12_BITS;                                // Remainder 12 bits
            return (remove_result_utf8_t){.state = {.state_type = CUTF_STATE_U8_2, .value = remainder}, .out = leading};
        }
        // Will it be four units?
        if (c < UTF8_MAX_FOUR_UNITS)
        {
            // Split into the leading part and the remainder
            const char8_t leading = (char8_t)UTF8_PREFIX_FOUR_UNITS | (char8_t)(c >> 18); // Leading
            auto const remainder = c & MASK_BOTTOM_18_BITS;                               // Remaining 18 bits
            return (remove_result_utf8_t){.state = {.state_type = CUTF_STATE_U8_3, .value = remainder}, .out = leading};
        }

        // Invalid
        return (remove_result_utf8_t){.state = {.state_type = CUTF_STATE_ERROR}};
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
        return (remove_result_utf8_t){.state = {.state_type = CUTF_STATE_ERROR}};
    }

    return (remove_result_utf8_t){.state = {.state_type = new_type, .value = v}, .out = out};
}

cutf_result_t cutf_s32tos8(const size_t sz_in, const char32_t p_in[const static sz_in], const size_t sz_out,
                           size_t *const p_consumed, char8_t p_out[const sz_out], size_t *const p_written,
                           cutf_state_t *const state)
{
    size_t pos_out, pos_in;
    for (pos_out = 0, pos_in = 0; pos_out < sz_out; ++pos_out)
    {
        // Advance the state
        remove_result_utf8_t res;
        if (state->state_type != CUTF_STATE_CLEAR)
        {
            // We do not need to read any more characters at the moment
            res = update_utf8_state_removing(*state, 0);
        }
        else
        {
            // We need to read the next codepoint
            if (pos_in >= sz_in)
            {
                break;
            }
            auto const c = p_in[pos_in];
            res = update_utf8_state_removing(*state, c);
            pos_in += (res.state.state_type != CUTF_STATE_ERROR);
        }

        // Was the state possible to advance?
        if (res.state.state_type == CUTF_STATE_ERROR)
        {
            return CUTF_INVALID_INPUT;
        }

        // Update the state with the new state and write out the next byte
        *state = res.state;
        p_out[pos_out] = res.out;
    }
    *p_consumed = pos_in;
    *p_written = pos_out;

    if (pos_in != sz_in)
        return CUTF_INSUFFICIENT_BUFFER;

    return CUTF_SUCCESS;
}

size_t cutf_count_s8asc32_complete(const size_t sz_in, const char8_t p_in[const static sz_in])
{
    size_t completed = 0;
    // for (size_t i = 0; i < sz_in; ++completed)
    // {
    //     auto const c = p_in[i];
    //     // Is it one unit?
    //     if (c < UTF8_PREFIX_TWO_UNITS)
    //     {
    //         i += 1;
    //     }
    //     // Is it two units?
    //     else if (c < UTF8_PREFIX_THREE_UNITS)
    //     {
    //         i += 2;
    //     }
    //     // Is it three units?
    //     else if (c < UTF8_PREFIX_FOUR_UNITS)
    //     {
    //         i += 3;
    //     }
    //     // Is it four units?
    //     else // if (c < UTF8_MAX_FOUR_UNITS)
    //     {
    //         i += 4;
    //     }
    // }
    for (unsigned i = 0; i < sz_in; ++i)
    {
        // Just increment if we do not have a continuation prefix, duh!
        completed += ((p_in[i] & 0xC0) != UTF8_PREFIX_CONTINUATION);
    }

    return completed;
}

static bool check_utf8_continuation_unit(char8_t const c)
{
    return (c & 0xC0) == UTF8_PREFIX_CONTINUATION;
}

cutf_result_t cutf_c8_next_unicode(const size_t sz_in, const char8_t p_in[const static sz_in], size_t *const p_consumed)
{
    size_t needed_continuations;
    switch (utf8_classify_leading_byte(p_in[0]))
    {
    case CUTF_STATE_U8_3:
        needed_continuations = 3;
        break;

    case CUTF_STATE_U8_2:
        needed_continuations = 2;
        break;

    case CUTF_STATE_U8_1:
        needed_continuations = 1;
        break;

    case CUTF_STATE_CLEAR:
        needed_continuations = 0;
        break;

    default:
        return CUTF_INVALID_INPUT;
    }

    if (needed_continuations + 1 > sz_in)
        return CUTF_INCOMPLETE_INPUT;

    for (unsigned j = 0; j < needed_continuations; ++j)
    {
        if (!check_utf8_continuation_unit(p_in[j + 1]))
        {
            return CUTF_INVALID_INPUT;
        }
    }

    *p_consumed = 1 + needed_continuations;
    return CUTF_SUCCESS;
}

cutf_result_t cutf_is_utf8_valid(const size_t sz_in, const char8_t p_in[const static sz_in], size_t *valid_count)
{
    size_t unused;
    // This function works the same as ``cutf_count_s8asc32``, but not caring about the number of Unicode codepoints
    return cutf_count_s8asc32(sz_in, p_in, valid_count, &unused);
}

cutf_result_t cutf_count_s8asc32(const size_t sz_in, const char8_t p_in[const static sz_in], size_t *const valid_count,
                                 size_t *const p_count)
{
    size_t pos_in = 0, pos_out = 0;
    cutf_result_t res = CUTF_SUCCESS;
    while (pos_in < sz_in)
    {
        size_t consumed;
        res = cutf_c8_next_unicode(sz_in - pos_in, p_in + pos_in, &consumed);

        if (res != CUTF_SUCCESS)
        {
            break;
        }
        pos_in += consumed;
        pos_out += 1;
    }

    *p_count = pos_out;
    *valid_count = pos_in;
    return res;
}

typedef enum
{
    UTF16_SURROGATE_HIGH_START = 0xD800,
    UTF16_SURROGATE_LOW_START = 0xDC00,
    UTF16_SURROGATE_PAIR_START = 0x10000,
    UTF16_SURROGATE_HIGH_END = 0xDBFF,
    UTF16_SURROGATE_LOW_END = 0xDFFF
} utf16_constants_t;

static cutf_state_t utf16_extract_leading_unit(const char16_t c)
{
    // Is just a single unit?
    if (c < UTF16_SURROGATE_HIGH_START || c > UTF16_SURROGATE_LOW_END)
    {
        // Nothing more to do
        return (cutf_state_t){.state_type = CUTF_STATE_CLEAR, .value = c};
    }
    // Is it the high surrogate (since it comes first)?
    if (c < UTF16_SURROGATE_HIGH_END && c >= UTF16_SURROGATE_HIGH_START)
    {
        return (cutf_state_t){.state_type = CUTF_STATE_U16_1, .value = MASK_BOTTOM_10_BITS & c};
    }

    // We either got the low surrogate or a value that's out of bounds either way.
    return (cutf_state_t){.state_type = CUTF_STATE_ERROR};
}

static cutf_state_t update_utf16_state_adding(const char16_t c, const cutf_state_t state)
{
    assert(state.state_type == CUTF_STATE_CLEAR || state.state_type == CUTF_STATE_U16_1);

    if (state.state_type == CUTF_STATE_CLEAR)
    {
        // We have a new one to deal with
        return utf16_extract_leading_unit(c);
    }

    // Check that this is indeed a low surrogate unit
    if (c > UTF16_SURROGATE_LOW_END || c < UTF16_SURROGATE_LOW_START)
        return (cutf_state_t){.state_type = CUTF_STATE_ERROR};

    if (state.state_type != CUTF_STATE_U16_1)
    {
        return (cutf_state_t){.state_type = CUTF_STATE_ERROR};
    }

    return (cutf_state_t){.state_type = CUTF_STATE_CLEAR,
                          .value = UTF16_SURROGATE_PAIR_START + (state.value << 10) + (MASK_BOTTOM_10_BITS & c)};
}

cutf_result_t cutf_c16toc32(const size_t sz_in, const char16_t p_in[const static sz_in], size_t *const p_consumed,
                            char32_t *const p_out, cutf_state_t *const state)
{
    cutf_result_t res = CUTF_INCOMPLETE_INPUT;
    size_t i;
    // Keep going until we either run out of characters or conversion is no longer incomplete.
    for (i = 0; i < sz_in; ++i)
    {
        auto const c = p_in[i];

        // Update the state type
        auto const new_state = update_utf16_state_adding(c, *state);
        if (new_state.state_type == CUTF_STATE_ERROR)
            return CUTF_INVALID_INPUT;

        // Update the value accumulated thus far
        *state = new_state;
        if (new_state.state_type == CUTF_STATE_CLEAR)
        {
            // We are done with parsing
            *p_out = new_state.value;
            // Clear the context value as well
            state->value = 0;
            res = CUTF_SUCCESS;
            i += 1;
            break;
        }
    }

    *p_consumed = i;
    return res;
}

cutf_result_t cutf_s16tos32(const size_t sz_in, const char16_t p_in[const static sz_in], const size_t sz_out,
                            size_t *const p_consumed, char32_t p_out[const sz_out], size_t *const p_written,
                            cutf_state_t *const state)
{
    size_t pos_in = 0, pos_out = 0;
    while (pos_in < sz_in && pos_out < sz_out)
    {
        size_t consumed;
        auto const res = cutf_c16toc32(sz_in - pos_in, p_in + pos_in, &consumed, p_out + pos_out, state);
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
    if (pos_in != sz_in)
        return CUTF_INSUFFICIENT_BUFFER;

    // Nope, we finished it all!
    return CUTF_SUCCESS;
}

typedef struct
{
    cutf_state_t state;
    char16_t out;
} remove_result_utf16_t;

static remove_result_utf16_t update_utf16_state_removing(const cutf_state_t state, const char32_t c)
{
    assert(state.state_type == CUTF_STATE_CLEAR || state.state_type == CUTF_STATE_U16_1);

    if (state.state_type == CUTF_STATE_CLEAR)
    {
        // Initialize the state
        // Check for valid Unicode codepoint
        if (!is_valid_unicode_codepoint(c))
            return (remove_result_utf16_t){.state = {.state_type = CUTF_STATE_ERROR}};

        // Does it fit in the single unit?
        if (c < UTF16_SURROGATE_PAIR_START)
            return (remove_result_utf16_t){.state = {.state_type = CUTF_STATE_CLEAR}, .out = (char16_t)c};

        // Return the high part and keep the low part in the state for later
        auto const adjusted = c - UTF16_SURROGATE_PAIR_START;
        auto const low_part = (char16_t)(adjusted & MASK_BOTTOM_10_BITS);
        return (remove_result_utf16_t){.state = {.state_type = CUTF_STATE_U16_1, .value = low_part},
                                       .out = UTF16_SURROGATE_HIGH_START | (adjusted >> 10)};
    }

    // The current state is not clean, which means we return the 10 bits that are left.
    // This also means we do not need "c"

    if (state.state_type != CUTF_STATE_U16_1)
    {
        return (remove_result_utf16_t){.state = {.state_type = CUTF_STATE_ERROR}};
    }

    auto const out = UTF16_SURROGATE_LOW_START | state.value;
    return (remove_result_utf16_t){.state = {.state_type = CUTF_STATE_CLEAR}, .out = out};
}

cutf_result_t cutf_s32tos16(const size_t sz_in, const char32_t p_in[const static sz_in], const size_t sz_out,
                            size_t *const p_consumed, char16_t p_out[const sz_out], size_t *const p_written,
                            cutf_state_t *const state)
{
    size_t pos_out, pos_in;
    for (pos_out = 0, pos_in = 0; pos_out < sz_out; ++pos_out)
    {
        // Advance the state
        remove_result_utf16_t res;
        if (state->state_type != CUTF_STATE_CLEAR)
        {
            // We do not need to read any more characters at the moment
            res = update_utf16_state_removing(*state, 0);
        }
        else
        {
            // We need to read the next codepoint
            if (pos_in >= sz_in)
            {
                break;
            }
            auto const c = p_in[pos_in];
            res = update_utf16_state_removing(*state, c);
            pos_in += (res.state.state_type != CUTF_STATE_ERROR);
        }

        // Was the state possible to advance?
        if (res.state.state_type == CUTF_STATE_ERROR)
        {
            return CUTF_INVALID_INPUT;
        }

        // Update the state with the new state and write out the next byte
        *state = res.state;
        p_out[pos_out] = res.out;
    }
    *p_consumed = pos_in;
    *p_written = pos_out;

    // Check if we ran out of output buffer before the end of the input
    if (pos_in != sz_in)
        return CUTF_INSUFFICIENT_BUFFER;

    // Nope, we finished it all!
    return CUTF_SUCCESS;
}

typedef enum
{
    BOM_UTF16_NATIVE = 0xFEFF,
    BOM_UTF16_REVERSE = 0xFFFE,
    BOM_UTF32_NATIVE = 0x0000FEFF,
    BOM_UTF32_REVERSE = 0xFFFE0000,
} endianness_bom_values;

utf_endianness_t cutf_utf16_bom_endianness(const char16_t bom)
{
    switch (bom)
    {
    case 0xFEFF:
        return CUTF_ENDIANNESS_NATIVE;

    case 0xFFFE:
        return CUTF_ENDIANNESS_REVERSE;

    default:
        return CUTF_ENDIANNESS_INVALID;
    }
}

char16_t cutf_utf16_bom(const utf_endianness_t endianness)
{
    switch (endianness)
    {
    case CUTF_ENDIANNESS_NATIVE:
        return BOM_UTF16_NATIVE;
    case CUTF_ENDIANNESS_REVERSE:
        return BOM_UTF16_REVERSE;
    default:
        return 0;
    }
}

typedef union {
    char16_t c16;
    struct
    {
        uint8_t b1, b2;
    };
} swap_endian_16_t;

void cutf_utf16_swap_endianness(const size_t sz_out, char16_t p_out[const sz_out],
                                const char16_t p_in[const static sz_out])
{
    for (size_t i = 0; i < sz_out; ++i)
    {
        // Read in the input value
        swap_endian_16_t const in = {.c16 = p_in[i]};
        // Reverse the byte order
        swap_endian_16_t const swapped = {.b1 = in.b2, .b2 = in.b1};
        // Write out the result
        p_out[i] = swapped.c16;
    }
}

utf_endianness_t cutf_utf32_bom_endianness(const char32_t bom)
{
    switch (bom)
    {
    case BOM_UTF32_NATIVE:
        return CUTF_ENDIANNESS_NATIVE;

    case BOM_UTF32_REVERSE:
        return CUTF_ENDIANNESS_REVERSE;

    default:
        return CUTF_ENDIANNESS_INVALID;
    }
}

typedef union {
    char32_t c32;
    struct
    {
        uint8_t b1, b2, b3, b4;
    };
} swap_endian_32_t;

void cutf_utf32_swap_endianness(const size_t sz_out, char32_t p_out[const sz_out],
                                const char32_t p_in[const static sz_out])
{
    for (size_t i = 0; i < sz_out; ++i)
    {
        swap_endian_32_t const in = {.c32 = p_in[i]};
        swap_endian_32_t const swapped = {.b1 = in.b4, .b2 = in.b3, .b3 = in.b2, .b4 = in.b1};
        p_out[i] = swapped.c32;
    }
}

cutf_result_t cutf_s8tos16(const size_t sz_in, const char8_t p_in[const static sz_in], const size_t sz_out,
                           size_t *const p_consumed, char16_t p_out[const sz_out], size_t *const p_written,
                           cutf_state_t *const state)
{
    size_t pos_out = 0, pos_in, consumed, written;
    // There are two options:
    // 1. We are in the middle of writing out the UTF-16 output
    // 2. We are in the middle of reading the UTF-8 input
    if (state->state_type == CUTF_STATE_U16_1)
    {
        // We are in the middle of writing out
        auto const res = update_utf16_state_removing(*state, 0);
        if (res.state.state_type == CUTF_STATE_ERROR)
        {
            return CUTF_INVALID_INPUT;
        }
        // Update state
        *state = res.state;
        // Write output
        p_out[0] = res.out;
        pos_out = 1;
    }

    cutf_result_t res = CUTF_SUCCESS;
    for (pos_in = 0; pos_in < sz_in && pos_out < sz_out; pos_in += consumed, pos_out += written)
    {
        // Consume UTF-8 character
        char32_t next_codepoint;
        res = cutf_c8toc32(sz_in - pos_in, p_in + pos_in, &consumed, &next_codepoint, state);
        if (res == CUTF_INCOMPLETE_INPUT)
        {
            pos_in += consumed;
            break;
        }

        if (res != CUTF_SUCCESS)
        {
            return res;
        }

        // Write the output as UTF-16
        size_t unused;
        res = cutf_s32tos16(1, &next_codepoint, sz_out - pos_out, &unused, p_out + pos_out, &written, state);
        if (res == CUTF_INSUFFICIENT_BUFFER)
        {
            pos_out += written;
            break;
        }

        if (res != CUTF_SUCCESS)
        {
            return res;
        }
    }

    *p_consumed = pos_in;
    *p_written = pos_out;
    return res;
}

cutf_result_t cutf_s16tos8(const size_t sz_in, const char16_t p_in[const static sz_in], const size_t sz_out,
                           size_t *const p_consumed, char8_t p_out[const sz_out], size_t *const p_written,
                           cutf_state_t *const state)
{
    size_t pos_out = 0, pos_in, consumed, written;
    while (pos_out < sz_out && (state->state_type == CUTF_STATE_U8_1 || state->state_type == CUTF_STATE_U8_2 ||
                                state->state_type == CUTF_STATE_U8_3))
    {
        // We are in the middle of writing out
        auto const res = update_utf8_state_removing(*state, 0);
        if (res.state.state_type == CUTF_STATE_ERROR)
        {
            return CUTF_INVALID_INPUT;
        }
        // Update state
        *state = res.state;
        // Write output
        p_out[pos_out] = res.out;
        pos_out += 1;
    }

    cutf_result_t res = CUTF_SUCCESS;
    for (pos_in = 0; pos_in < sz_in && pos_out < sz_out; pos_in += consumed, pos_out += written)
    {
        // Consume UTF-16 characters and decode into UTF-32
        char32_t next_codepoint;
        res = cutf_c16toc32(sz_in - pos_in, p_in + pos_in, &consumed, &next_codepoint, state);
        if (res == CUTF_INCOMPLETE_INPUT)
        {
            pos_in += consumed;
            break;
        }

        if (res != CUTF_SUCCESS)
        {
            return res;
        }

        // Write the output as UTF-8
        size_t unused;
        res = cutf_s32tos8(1, &next_codepoint, sz_out - pos_out, &unused, p_out + pos_out, &written, state);
        if (res == CUTF_INSUFFICIENT_BUFFER)
        {
            pos_out += written;
            break;
        }

        if (res != CUTF_SUCCESS)
        {
            return res;
        }
    }

    *p_consumed = pos_in;
    *p_written = pos_out;
    return res;
}
