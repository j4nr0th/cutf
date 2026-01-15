#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <uchar.h>

enum cutf_result_t
{
    CUTF_SUCCESS,          // Success
    CUTF_INCOMPLETE_INPUT, // Input string did not contain enough characters to complete conversion of the last output
    CUTF_INSUFFICIENT_BUFFER, // Output buffer was too small to fit all output characters
    CUTF_INVALID_INPUT,       // Input string was not in the valid encoding
};
typedef enum cutf_result_t cutf_result_t;

enum cutf_state_type_t
{
    CUTF_STATE_CLEAR, // Nothing currently in the state
    CUTF_STATE_U8_1,  // Parsing UTF8, 1 surrogate remaining
    CUTF_STATE_U8_2,  // Parsing UTF8, 2 surrogates remaining
    CUTF_STATE_U8_3,  // Parsing UTF8, 3 surrogates remaining
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

bool cutf_state_is_clean(cutf_state_t state);

static const cutf_state_t CUTF_STATE_INITIALIZER = {.state_type = CUTF_STATE_CLEAR};

/**
 * Extract the first Unicode point from a UTF-8 string.
 *
 * @param sz_in Maximum number of UTF-8 units that can be read.
 * @param p_in Input UTF-8 string.
 * @param p_consumed Pointer which receives the number of UTF-8 units converted.
 * @param p_out Pointer which receives the first Unicode codepoint as UTF-32.
 * @param state Pointer to the conversion state.
 * @return CUTF_SUCCESS if successful, otherwise an error code.
 */
cutf_result_t cutf_c8toc32(size_t sz_in, const char8_t p_in[static sz_in], size_t *p_consumed, char32_t *p_out,
                           cutf_state_t *state);

/**
 * Convert a UTF-8 string to a UTF-32 string.
 *
 * @param sz_in Number of UTF-8 units to convert.
 * @param p_in Input UTF-8 string to convert.
 * @param sz_out Size of the output string.
 * @param p_consumed Pointer which receives the number of UTF-8 units converted.
 * @param p_out Pointer to the output array.
 * @param p_written Pointer which receives the number of UTF-32 units written.
 * @param state Pointer to the conversion state.
 * @return CUTF_SUCCESS if successful, otherwise an error code.
 */
cutf_result_t cutf_s8tos32(size_t sz_in, const char8_t p_in[static sz_in], size_t sz_out, size_t *p_consumed,
                           char32_t p_out[sz_out], size_t *p_written, cutf_state_t *state);

/**
 * Convert a UTF-32 string to a UTF-8 string.
 *
 * @param sz_in Number of characters in the input.
 * @param p_in Input UTF-32 string to convert.
 * @param sz_out Size of the output string.
 * @param p_consumed Pointer which receives the number of UTF-32 units consumed.
 * @param p_out Pointer to the output array.
 * @param p_written Pointer which receives the number of UTF-8 units written.
 * @param state Pointer to the conversion state.
 * @return CUTF_SUCCESS if successful, otherwise an error code.
 */
cutf_result_t cutf_s32tos8(size_t sz_in, const char32_t p_in[const static sz_in], size_t sz_out, size_t *p_consumed,
                           char8_t p_out[const sz_out], size_t *p_written, cutf_state_t *state);

/**
 * Count the number of UTF-32 characters required to represent all characters in the input, assuming the input is
 * a complete and correctly encoded UTF-8 string.
 *
 * @param sz_in Length of the UTF-8 string.
 * @param p_in Pointer to the UTF-8 string.
 * @return Number of UTF-32 units needed to represent the input.
 */
size_t cutf_count_s8asc32_complete(size_t sz_in, const char8_t p_in[static sz_in]);

/**
 *
 * @param sz_in Number of UTF-8 units in the input to check.
 * @param p_in Input string to check if it is a valid UTF-8 encoded string.
 * @param valid_count Pointer that receives the number of UTF-8 units from the start of the input.
 * @return CUTF_SUCCESS if the input is a valid UTF-8 string. When the string is
 *         valid, but the last codepoint is missing surrogate units, CUTF_INCOMPLETE_INPUT
 *         is returned. If the encoding is not UTF-8 encoded, CUTF_INVALID_INPUT is returned.
 */
cutf_result_t cutf_is_utf8_valid(size_t sz_in, const char8_t p_in[static sz_in], size_t *valid_count);

/**
 *
 * @param sz_in Number of UTF-8 units in the input.
 * @param p_in Input string to advance along.
 * @param p_consumed Pointer which receives the number of UTF-8 units to advance to the next Unicode unit.
 * @return CUTF_SUCCESS if successful, otherwise an error code.
 */
cutf_result_t cutf_c8_next_unicode(size_t sz_in, const char8_t p_in[const static sz_in], size_t *p_consumed);

/**
 * Count the number of UTF-32 characters required to represent all characters in the input.
 *
 * @param sz_in Length of the UTF-8 string.
 * @param p_in Pointer to the UTF-8 string.
 * @param valid_count Number of UTF-8 units that can be converted into complete UTF-32 codepoints.
 * @param p_count Pointer that receives the number of UTF-32 units needed to represent the input string.
 * @return CUTF_SUCCESS on success, otherwise an error code.
 */
cutf_result_t cutf_count_s8asc32(size_t sz_in, const char8_t p_in[static sz_in], size_t *valid_count, size_t *p_count);

/**
 * Extract the first Unicode point from a UTF-16 string.
 *
 * @param sz_in Maximum number of UTF-16 units that can be read.
 * @param p_in Input UTF-16 string.
 * @param p_consumed Pointer which receives the number of UTF-16 units converted.
 * @param p_out Pointer which receives the first Unicode codepoint as UTF-32.
 * @param state Pointer to the conversion state.
 * @return CUTF_SUCCESS if successful, otherwise an error code.
 */
cutf_result_t cutf_c16toc32(size_t sz_in, const char16_t p_in[static sz_in], size_t *p_consumed, char32_t *p_out,
                            cutf_state_t *state);

/**
 * Convert a UTF-16 string to a UTF-32 string.
 *
 * @param sz_in Number of UTF-16 units to convert.
 * @param p_in Input UTF-16 string to convert.
 * @param sz_out Size of the output string.
 * @param p_consumed Pointer which receives the number of UTF-16 units converted.
 * @param p_out Pointer to the output array.
 * @param p_written Pointer which receives the number of UTF-32 units written.
 * @param state Pointer to the conversion state.
 * @return CUTF_SUCCESS if successful, otherwise an error code.
 */
cutf_result_t cutf_s16tos32(size_t sz_in, const char16_t p_in[const static sz_in], size_t sz_out, size_t *p_consumed,
                            char32_t p_out[const sz_out], size_t *p_written, cutf_state_t *state);

/**
 * Convert a UTF-32 string to a UTF-16 string.
 *
 * @param sz_in Number of characters in the input.
 * @param p_in Input UTF-32 string to convert.
 * @param sz_out Size of the output string.
 * @param p_consumed Pointer which receives the number of UTF-32 units consumed.
 * @param p_out Pointer to the output array.
 * @param p_written Pointer which receives the number of UTF-16 units written.
 * @param state Pointer to the conversion state.
 * @return CUTF_SUCCESS if successful, otherwise an error code.
 */
cutf_result_t cutf_s32tos16(size_t sz_in, const char32_t p_in[const static sz_in], size_t sz_out, size_t *p_consumed,
                            char16_t p_out[const sz_out], size_t *p_written, cutf_state_t *state);

typedef enum
{
    CUTF_ENDIANNESS_INVALID = -1,
    CUTF_ENDIANNESS_NATIVE = 0,
    CUTF_ENDIANNESS_REVERSE = +1,
} utf_endianness_t;

/**
 * Determine the endianness of a UTF-16 string based on a byte-order mark (BOM).
 *
 * @param bom Byte-order mark (BOM) codepoint U+FEFF encoded.
 * @return Indicator of UTF-16 endianness and zero if the passed value was not BOM.
 */
utf_endianness_t cutf_utf16_bom_endianness(char16_t bom);

/**
 * Retrieves the Byte Order Mark (BOM) value for UTF-16 encoding based on the provided endianness.
 *
 * @param endianness The specified UTF endianness, either native, reverse.
 * @return The UTF-16 BOM corresponding to the specified endianness. Returns 0 if the provided endianness is invalid.
 */
char16_t cutf_utf16_bom(utf_endianness_t endianness);

/**
 * Reverse the endianness of UTF-16 units in the array. Input and output may overlap.
 *
 * @param sz_out Number of UTF-16 units to reverse the endianness of.
 * @param p_out Array which receives the resulting codepoints.
 * @param p_in Input UTF-16 string.
 */
void cutf_utf16_swap_endianness(size_t sz_out, char16_t p_out[sz_out], const char16_t p_in[static sz_out]);

/**
 * Determine the endianness of a UTF-32 string based on a byte-order mark (BOM).
 *
 * @param bom Byte-order mark (BOM) codepoint U+FEFF encoded.
 * @return Indicator of UTF-32 endianness and zero if the passed value was not BOM.
 */
utf_endianness_t cutf_utf32_bom_endianness(char32_t bom);

/**
 * Reverse the endianness of UTF-32 units in the array. Input and output may overlap.
 *
 * @param sz_out Number of UTF-32 units to reverse the endianness of.
 * @param p_out Array which receives the resulting codepoints.
 * @param p_in Input UTF-32 string.
 */
void cutf_utf32_swap_endianness(size_t sz_out, char32_t p_out[sz_out], const char32_t p_in[static sz_out]);

/**
 * Convert a UTF-8 string to a UTF-16 string.
 *
 * @param sz_in Number of characters in the input.
 * @param p_in Input UTF-8 string to convert.
 * @param sz_out Size of the output string.
 * @param p_consumed Pointer which receives the number of UTF-8 units consumed.
 * @param p_out Pointer to the output array.
 * @param p_written Pointer which receives the number of UTF-16 units written.
 * @param state Pointer to the conversion state.
 * @return CUTF_SUCCESS if successful, otherwise an error code.
 */
cutf_result_t cutf_s8tos16(size_t sz_in, const char8_t p_in[static sz_in], size_t sz_out, size_t *p_consumed,
                           char16_t p_out[sz_out], size_t *p_written, cutf_state_t *state);

/**
 * Convert a UTF-16 string to a UTF-8 string.
 *
 * @param sz_in Number of characters in the input.
 * @param p_in Input UTF-16 string to convert.
 * @param sz_out Size of the output string.
 * @param p_consumed Pointer which receives the number of UTF-16 units consumed.
 * @param p_out Pointer to the output array.
 * @param p_written Pointer which receives the number of UTF-8 units written.
 * @param state Pointer to the conversion state.
 * @return CUTF_SUCCESS if successful, otherwise an error code.
 */
cutf_result_t cutf_s16tos8(size_t sz_in, const char16_t p_in[static sz_in], size_t sz_out, size_t *p_consumed,
                           char8_t p_out[sz_out], size_t *p_written, cutf_state_t *state);
