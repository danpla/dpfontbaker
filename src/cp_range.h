
#pragma once

#include <stdexcept>
#include <vector>

#include "unicode.h"


namespace dpfb {
namespace cp_range {


class CpRangeError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};


struct CpRange {
    // [cpFirst, cpLast]
    char32_t cpFirst;
    char32_t cpLast;

    CpRange()
        : cpFirst {}
        , cpLast {}
    {}

    explicit CpRange(char32_t cpFirst)
        : cpFirst {cpFirst}
        , cpLast {cpFirst}
    {}

    CpRange(char32_t cpFirst, char32_t cpLast)
        : cpFirst {cpFirst}
        , cpLast {cpLast}
    {}
};


inline bool operator==(const CpRange& a, const CpRange& b)
{
    return a.cpFirst == b.cpFirst && a.cpLast == b.cpLast;
}


inline bool operator!=(const CpRange& a, const CpRange& b)
{
    return !(a == b);
}


using CpRangeList = std::vector<CpRange>;


/**
 * Parse code point ranges from string.
 *
 * The input string can contain a comma-separated list of single code
 * points and code point ranges [first, last]. Code points within a
 * range are separated by -. A code point can be either a decimal
 * number or U+ followed by a hex sequence. For example:
 *
 *     U+20-126, U+0080-U+00FF, 9786, U+FFFC
 *
 * The function doesn't perform code point validation.
 *
 * \returns CpRangeList containing ranges in the same order as
 *     in the string, possibly with duplicates and overlaps
 *
 * \throws cp_range::CpRangeError on errors
 */
CpRangeList parse(const char* str);


/**
 * Compress ranges.
 *
 * The function merges overlapping ranges (regardless of their
 * order), effectively removing all duplicate code points.
 */
void compress(CpRangeList& cpRangeList);


}
}
