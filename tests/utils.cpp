
#include "utils.h"

#include <cctype>
#include <cstring>

#include "catch.hpp"


const char commentStart = '#';


static bool isEnd(char c)
{
    return c == commentStart || c == 0;
}


void splitKeyValue(char* str, char*& key, char*& value)
{
    char* s = str;
    while (std::isspace(*s))
        ++s;

    key = s;

    if (isEnd(*s)) {
        // No key
        *s = 0;
        value = s;
        return;
    }

    ++s;
    while (!std::isspace(*s) && !isEnd(*s))
        ++s;

    if (isEnd(*s)) {
        // No whitespace after key
        *s = 0;
        value = s;
        return;
    }

    *s = 0;  // Key end
    ++s;
    while (std::isspace(*s))
        ++s;
    value = s;

    if (isEnd(*s))
        // No value
        return;

    char* lastValueChar = s;
    while (!isEnd(*s)) {
        if (!std::isspace(*s))
            lastValueChar = s;
        ++s;
    }

    ++lastValueChar;
    *lastValueChar = 0;
}


TEST_CASE("splitKeyValue", "[test_utils]") {
    struct Test {
        const char* str;
        const char* key;
        const char* value;
    };

    Test tests[] = {
        {"", "", ""},
        {"# Comment", "", ""},
        {" \t # Comment", "", ""},
        {"key", "key", ""},
        {"key \t ", "key", ""},
        {" \t key \t foo \t bar \t # Comment", "key", "foo \t bar"},
    };

    for (const auto& test : tests) {
        char str[128];
        REQUIRE(std::strlen(test.key) < sizeof(str));
        std::strcpy(str, test.str);

        char* key = nullptr;
        char* value = nullptr;
        splitKeyValue(str, key, value);

        REQUIRE(key);
        INFO('"' << key << "\" == \"" << test.key << '"');
        REQUIRE(std::strcmp(key, test.key) == 0);

        REQUIRE(value);
        INFO('"' << value << "\" == \"" << test.value << '"');
        REQUIRE(std::strcmp(value, test.value) == 0);
    }
}
