
#include "cp_range.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstddef>

#include "str.h"
#include "unicode.h"


namespace dpfb {
namespace cp_range {


const char cpRangeSeparator = ',';


static char32_t parseCp(const char*& str)
{
    const char* end;
    const auto result = unicode::strToCp(str, &end);
    if (end == str) {
        // Find a meaningful end of the invalid code point.
        end = str;
        while (const auto c = *end) {
            if (c == cpRangeSeparator || std::isspace(c))
                break;
            ++end;
        }

        throw CpRangeError(
            str::format(
                "Invalid code point specifier \"%.*s\"",
                static_cast<int>(end - str), str));
    }

    str = end;

    return result;
}


CpRangeList parse(const char* str)
{
    assert(str);

    CpRangeList result;

    while (*str) {
        while (std::isspace(*str))
            ++str;

        if (*str == cpRangeSeparator) {
            ++str;
            continue;
        }

        if (*str == 0)
            break;

        const auto cp1 = parseCp(str);

        while (std::isspace(*str))
            ++str;

        if (*str == cpRangeSeparator || *str == 0) {
            // A single code point.
            result.emplace_back(cp1, cp1);
            continue;
        }

        if (*str != '-')
            throw CpRangeError(
                str::format(
                    "Expected \"-\" after the range start, "
                    "but \"%c\" found", *str));

        ++str;
        while (std::isspace(*str))
            ++str;

        if (*str == cpRangeSeparator || *str == 0)
            throw CpRangeError("Unexpected end of the range");

        const auto cp2 = parseCp(str);
        if (cp1 > cp2)
            throw CpRangeError(
                str::format(
                    "Range start > range end (%s > %s)",
                    unicode::cpToStr(cp1),
                    unicode::cpToStr(cp2)));

        result.emplace_back(cp1, cp2);

        while (std::isspace(*str))
            ++str;

        if (*str == cpRangeSeparator || *str == 0)
            continue;

        throw CpRangeError(
            str::format(
                "Unexpected character at the end of the range: \"%c\"",
                *str));
    }

    return result;
}


void compress(CpRangeList& cpRangeList)
{
    if (cpRangeList.size() < 2)
        return;

    struct CmpRangesByFirstCp {
        bool operator()(const CpRange& a, const CpRange& b) const
        {
            return a.cpFirst < b.cpFirst;
        }
    };

    std::sort(cpRangeList.begin(), cpRangeList.end(), CmpRangesByFirstCp());

    std::size_t curIdx = 0;
    for (std::size_t nextIdx = 1; nextIdx < cpRangeList.size(); ++nextIdx) {
        auto& curRange = cpRangeList[curIdx];
        auto& nextRange = cpRangeList[nextIdx];

        if (curRange.cpLast + 1 < nextRange.cpFirst) {
            // [0, 1], [3, 4] - nextRange is next curRange
            ++curIdx;
            cpRangeList[curIdx] = nextRange;
        } else if (curRange.cpLast < nextRange.cpLast) {
            // [0, 1], [2, 3] - merge nextRange with curRange
            curRange.cpLast = nextRange.cpLast;
        } // else nextRange is in curRange:
          // [0, 1], [0, 1]
          // [0, 2], [0, 1]
          // [0, 2], [0, 2]
    }

    assert(curIdx < cpRangeList.size());
    cpRangeList.resize(curIdx + 1);
}


}
}
