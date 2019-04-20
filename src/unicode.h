
#pragma once

#include <string>


namespace unicode {


const char32_t maxCp = 0x10ffff;
const char32_t replacementCharacter = 0xfffd;


/**
 * Convert a code point to a U+ encoded string.
 */
const char* cpToStr(char32_t cp);


/**
 * Convert a string to a code point.
 *
 * The function accept strings in both U+ and decimal forms. If the
 * end parameter if not null, it will point to the past-the-last
 * parsed character. If the string is invalid, str == end, including
 * the case when there are no digits after "U+" prefix.
 *
 * Similar to strto*() routines, the function skips leading
 * whitespace and clamps the result to maximum possible value
 * (UINT_LEAST32_MAX), but doesn't set errno in the latter case.
 * No code point validation is performed.
 */
char32_t strToCp(const char* str, const char** end = nullptr);


/**
 * Convert UTF-16 to UTF-8.
 *
 * The function will replace invalid code points with the replacement
 * character (U+FFFD).
 */
std::string utf16ToUtf8(const char16_t* begin, const char16_t* end);


}
