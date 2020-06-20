
#include "catch.hpp"

#include <cstring>
#include <string>

#include "unicode.h"


using namespace dpfb::unicode;


TEST_CASE("cpToStr", "[unicode]") {
    struct Test {
        char32_t cp;
        const char* str;
    };

    const Test tests[] = {
        {0x0000, "U+0000"},
        {0x0001, "U+0001"},
        {0xabcde, "U+ABCDE"},

        {0xd800, "U+D800"},
        {0xdfff, "U+DFFF"},
        {0x110000, "U+110000"},
        {0xffffffff, "U+FFFFFFFF"},
    };

    for (const auto& test : tests) {
        INFO(cpToStr(test.cp) << " == " << test.str);
        REQUIRE(std::strcmp(cpToStr(test.cp), test.str) == 0);
    }
}


TEST_CASE("strToCp", "[unicode]") {
    struct Test {
        const char* str;
        char32_t cp;
        int endOffset;

        Test(const char* str, char32_t cp, int endOffset = -1)
            : str {str}
            , cp {cp}
            , endOffset {endOffset}
        {
            if (this->endOffset == -1)
                this->endOffset = std::strlen(str);
        }
    };

    const Test tests[] = {
        {"U+0000", 0x0000},
        {"U+0001", 0x0001},
        {"U+1", 0x0001},
        {"U+ABCDE", 0xabcde},
        {"U+AbCdE", 0xabcde},
        {" \tU+AbCdE", 0xabcde},

        {"U+D800", 0xd800},
        {"U+DFFF", 0xdfff},

        {"U+110000", 0x110000},
        {"U+FFFFFFFF", 0xffffffff},
        {"U+FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", UINT_LEAST32_MAX},
        {
            " \tU+FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFzzz",
            UINT_LEAST32_MAX,
            36
        },

        {"", 0},
        {" \t", 0, 0},
        {"0", 0},
        {"0x1", 0, 1},
        {"0X1", 0, 1},
        {" \t0X1", 0, 3},
        {"U+0x1", 0, 3},
        {"U+0X1", 0, 3},
        {"077", 77},  // Not octal
        {"55296", 55296},  // U+D800
        {"57343", 57343},  // U+DFFF
        {"1114111", 1114111},  // U+10FFFF
        {"1114112", 1114112},  // U+110000
        {"9999999999999999999999999999999999", UINT_LEAST32_MAX},
        {
            " \t9999999999999999999999999999999999zzz",
            UINT_LEAST32_MAX,
            36
        },
    };

    for (const auto& test : tests) {
        INFO(test.str);

        const char* end;
        REQUIRE(strToCp(test.str, &end) == test.cp);

        REQUIRE(end - test.str == test.endOffset);
    }

    const char* invalidStrings[] = {
        "U",
        "U+",
        "U+ 1",
        "U+ 0x1",
        "U+ 0X1",
        " \tU+ 0X1",
        "U+g",
        "U-1",
        "u+0000",
        "U0000",
        "-1",
        "+1",
    };

    for (const auto* s : invalidStrings) {
        const char* end;
        REQUIRE(strToCp(s, &end) == 0);
        REQUIRE(end == s);
    }
}


TEST_CASE("utf16ToUtf8", "[unicode]") {
    const char* replacement = "\xef\xbf\xbd";

    struct Utf16ToUtf8Test {
        const char16_t* utf16str;
        const char* utf8str;
    };

    const Utf16ToUtf8Test testItems[] = {
        {u"\u0414", "\xd0\x94"},
        {u"\xd800\xdc00", "\xf0\x90\x80\x80"},
        {u"\xd800\xdfff", "\xf0\x90\x8f\xbf"},
        {u"\xd83d\xde00", "\xf0\x9f\x98\x80"},

        // Invalid surrogate combinations
        {u"\xd800", replacement},  // min leading
        {u"\xdbff", replacement},  // max leading
        {u"\xdc00", replacement},  // min trailing
        {u"\xdfff", replacement},  // max trailing
        {u"\xd800\xdbff", replacement},  // min leading + (min trailing - 1)
        {u"\xd800\xe000", replacement},  // min leading + (max trailing + 1)
        {u"\xdbff\xdbff", replacement},  // max leading + (min trailing - 1)
        {u"\xdbff\xe000", replacement},  // max leading + (max trailing + 1)
    };

    for (const auto& item : testItems) {
        REQUIRE(
            utf16ToUtf8(
                item.utf16str,
                item.utf16str
                    + std::char_traits<char16_t>::length(item.utf16str))
            == item.utf8str);
    }
}
