# `cutf` - C UTF Processing Library

The purpose of this library is to provide basic UTF-8, UTF-16, and UTF-32 conversion functions. The main motivation for
it was the current lack of support in the standard library (up until C29 at least). Another motivation for this was to
be able to deal with conversion between these formats without relying on locale settings.

## Capabilities

The library provides conversion functions for dealing with all conversions between UTF-8, UTF-16, and UTF-32. These are
all pure, thread-safe, and "restartable". Importantly, unlike the standard library's `mbstowcs` and similar functions,
these functions do not interract with the locale settings.

Some utility functions for text processing are also provided, namely functions for counting actual codepoints in UTF-8
and UTF-16 strings, and functions to advance to the next codepoint in these strings.

## Requirements

The library uses CMake as its build system and is most easily added as dependency using CMake's `add_subdirectory`
function. Besides that, to build it, a compiler that supports C23 is required. However, to use it, C99 is enough.