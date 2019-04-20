
#include "catch.hpp"

#include <cstdint>

#include "cp_range.h"
#include "unicode.h"


namespace cp_range {


static std::ostream& operator<<(std::ostream& os, const CpRange& cpr) {
    return os << "CpRange([" << cpr.cpFirst << ", " << cpr.cpLast << "])";
}


}


TEST_CASE("cp_range::compress") {
    using namespace cp_range;

    struct Test {
        CpRangeList list;
        const CpRangeList expected;
    };

    Test tests[] = {
        {{CpRange(0, 1), CpRange(2, 3), CpRange(4, 5)},
            {CpRange(0, 5)}},
        {{CpRange(0, 2), CpRange(2, 3)},
            {CpRange(0, 3)}},
        {{CpRange(0, 0), CpRange(1, 3)},
            {CpRange(0, 3)}},
        {{CpRange(1, 2), CpRange(0, 3)},
            {CpRange(0, 3)}},
        {{CpRange(0, 1), CpRange(3, 4)},
            {CpRange(0, 1), CpRange(3, 4)}},
        {{CpRange(3, 4), CpRange(0, 1), CpRange(0, 1)},
            {CpRange(0, 1), CpRange(3, 4)}},
        {{CpRange(3, 4), CpRange(0, 1), CpRange(0, 1), CpRange(2, 2)},
            {CpRange(0, 4)}},
    };

    for (auto& test : tests) {
        compress(test.list);
        REQUIRE(test.list == test.expected);
    }
}



TEST_CASE("cp_range::parse") {
    using namespace cp_range;

    // U+ format
    REQUIRE(parse("U+1234") == CpRangeList({CpRange(0x1234, 0x1234)}));
    REQUIRE(parse("U+aAfF") == CpRangeList({CpRange(0xaaff, 0xaaff)}));

    REQUIRE_THROWS_AS(parse("u+1234"), CpRangeError);
    REQUIRE_THROWS_AS(parse("U+"), CpRangeError);

    // Don't allow octal and hex conversion
    REQUIRE(parse("0123") == CpRangeList({CpRange(123, 123)}));
    REQUIRE_THROWS_AS(parse("0x123"), CpRangeError);

    // Don't allow negative numbers
    REQUIRE_THROWS_AS(parse("-123"), CpRangeError);
    REQUIRE_THROWS_AS(parse("-U+123"), CpRangeError);
    REQUIRE_THROWS_AS(parse("U+-123"), CpRangeError);

    // Clamp
    REQUIRE(
        parse("99999999999999999")
        == CpRangeList({CpRange(UINT_LEAST32_MAX, UINT_LEAST32_MAX)}));
    REQUIRE(
        parse("U+ffffffffffffffffff")
        == CpRangeList({CpRange(UINT_LEAST32_MAX, UINT_LEAST32_MAX)}));

    // Whitespace
    REQUIRE(parse("") == CpRangeList());
    REQUIRE(parse(" \t ") == CpRangeList());
    REQUIRE(parse(" , , ") == CpRangeList());
    REQUIRE(parse(" \t0 \t- \t1 \t") == CpRangeList({CpRange(0, 1)}));

    // Explicit start == end
    REQUIRE(parse("0-0, 2-2") == CpRangeList({CpRange(0, 0), CpRange(2, 2)}));

    // Start < end
    REQUIRE_THROWS_AS(parse("1-0"), CpRangeError);

    // Unexpected end of the range
    REQUIRE_THROWS_AS(parse("0-"), CpRangeError);
    REQUIRE_THROWS_AS(parse("0- "), CpRangeError);
    REQUIRE_THROWS_AS(parse("0- ,"), CpRangeError);

    // Invalid range separator
    REQUIRE_THROWS_AS(parse("0="), CpRangeError);

    // Unexpected character at the end of the range
    REQUIRE_THROWS_AS(parse("0a"), CpRangeError);
    REQUIRE_THROWS_AS(parse("0-1a"), CpRangeError);

    // Garbage
    REQUIRE_THROWS_AS(parse("foo"), CpRangeError);
    REQUIRE_THROWS_AS(parse("foo-1"), CpRangeError);
    REQUIRE_THROWS_AS(parse("1-foo"), CpRangeError);
}
