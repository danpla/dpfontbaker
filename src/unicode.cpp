
#include "unicode.h"

#include <cctype>
#include <cstdio>
#include <cstdint>
#include <cinttypes>


namespace unicode {


const char32_t leadingSurrogateMin = 0xd800;
const char32_t leadingSurrogateMax = 0xdbff;

const char32_t trailingSurrogateMin = 0xdc00;
const char32_t trailingSurrogateMax = 0xdfff;


const char32_t minSurrogate = leadingSurrogateMin;
const char32_t maxSurrogate = trailingSurrogateMax;


static inline bool isLeadingSurrogate(char32_t cp)
{
    return cp >= leadingSurrogateMin && cp <= leadingSurrogateMax;
}


static inline bool isTrailingSurrogate(char32_t cp)
{
    return cp >= trailingSurrogateMin && cp <= trailingSurrogateMax;
}


static inline bool isSurrogate(char32_t cp)
{
    return cp >= minSurrogate && cp <= maxSurrogate;
}


static inline char32_t combineSurrogates(char32_t leading, char32_t trailing)
{
    return 0x10000 + (((leading & 0x3ff) << 10) | (trailing & 0x3ff));
}


static inline bool isValidCp(char32_t cp)
{
    return cp <= maxCp && !isSurrogate(cp);
}


const char* cpToStr(char32_t cp)
{
    // char32_t is an alias to uint_least32_t and therefore may
    // have more than 32 bits.
    static char buf[2 + sizeof(char32_t) * 2 + 1];
    std::snprintf(buf, sizeof(buf), "U+%04" PRIXLEAST32, cp);
    return buf;
}


char32_t strToCp(const char* str, const char** end)
{
    // It's easier to roll our own routine than to use strtol(),
    // because otherwise we need:
    //    1. Ensure there is no leading -.
    //    2. Take into account that strtol() accepts optional "0x"
    //       or "0X" prefixes for base 16, which we should treat
    //       as 0.

    char32_t result = 0;

    const char* strStart = str;

    while (std::isspace(*str))
        ++str;

    int base;
    if (*str == 'U' && str[1] == '+') {
        base = 16;
        str += 2;
    } else
        base = 10;

    const auto max = UINT_LEAST32_MAX;
    const auto cutoff = max / base;
    const auto cutlimit = max % base;

    bool wasDigits = false;
    while (true) {
        unsigned char c = *str;

        if (c >= '0' && c <= '9')
            c -= '0';
        else if (base == 16) {
            if (c >= 'a' && c <= 'f')
                c -= 'a' - 10;
            else if (c >= 'A' && c <= 'F')
                c -= 'A' - 10;
            else
                break;
        } else
            break;

        wasDigits = true;
        ++str;

        if (result == max
                || result > cutoff
                || (result == cutoff && c > cutlimit))
            result = max;
        else {
            result *= base;
            result += c;
        }
    }

    if (end) {
        if (wasDigits)
            *end = str;
        else
            *end = strStart;
    }

    return result;
}


const std::string replacementCharacterUtf8 = "\xef\xbf\xbd";


static int utf8Length(char32_t cp)
{
    if (cp < 0x80)
        return 1;
    if (cp < 0x800)
        return 2;
    if (isSurrogate(cp))
        return replacementCharacterUtf8.length();
    if (cp < 0x10000)
        return 3;
    if (cp <= maxCp)
        return 4;

    return replacementCharacterUtf8.length();
}


static char* encodeUtf8(char32_t cp, char* out)
{
    if (cp < 0x80)
        *out++ = cp;
    else if (cp < 0x800) {
        *out++ = 0xc0 | (cp >> 6);
        *out++ = 0x80 | (cp & 0x3f);
    } else if (isSurrogate(cp)) {
        out += replacementCharacterUtf8.copy(
            out,
            replacementCharacterUtf8.length());
    } else if (cp < 0x10000) {
        *out++ = 0xe0 | (cp >> 12);
        *out++ = 0x80 | ((cp >> 6) & 0x3f);
        *out++ = 0x80 | (cp & 0x3f);
    } else if (cp <= maxCp) {
        *out++ = 0xf0 | (cp >> 18);
        *out++ = 0x80 | ((cp >> 12) & 0x3f);
        *out++ = 0x80 | ((cp >> 6) & 0x3f);
        *out++ = 0x80 | (cp & 0x3f);
    } else
        out += replacementCharacterUtf8.copy(
            out,
            replacementCharacterUtf8.length());

    return out;
}


std::string utf16ToUtf8(const char16_t* begin, const char16_t* end)
{
    std::string result;
    std::size_t resultLen = 0;

    if (begin < end)
        result.reserve(end - begin);

    while (begin < end) {
        char32_t cp = *begin++;
        // A lone surrogate is handled by utf8Length() and
        // encodeUtf8(), so we don't have to set cp to replacement
        // character.
        if (isLeadingSurrogate(cp) && begin < end) {
            const char32_t cp2 = *begin++;
            if (isTrailingSurrogate(cp2))
                cp = combineSurrogates(cp, cp2);
        }

        const auto seqIdx = resultLen;
        resultLen += utf8Length(cp);
        result.resize(resultLen);
        encodeUtf8(cp, &result[seqIdx]);
    }

    return result;
}


int encodeUtf16(char32_t cp, char16_t out[2])
{
    if (cp < 0x10000) {
        out[0] = cp;
        return 1;
    } else if (cp <= maxCp) {
        cp -= 0x10000;
        out[0] = leadingSurrogateMin + ((cp >> 10) & 0x3ff);
        out[1] = trailingSurrogateMin + (cp & 0x3ff);
        return 2;
    }

    out[0] = replacementCharacter;
    return 1;
}


}
