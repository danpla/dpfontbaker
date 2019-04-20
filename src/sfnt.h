
#pragma once

#include <cstdint>
#include <vector>

#include "streams/stream.h"


constexpr std::uint32_t sfntTag(char c1, char c2, char c3, char c4)
{
    return (
        (static_cast<std::uint32_t>(c1) << 24)
        | (static_cast<std::uint32_t>(c2) << 16)
        | (static_cast<std::uint32_t>(c3) << 8)
        | static_cast<std::uint32_t>(c4)
    );
}


const char* sfntTagToStr(std::uint32_t tag);


class SfntOffsetTable {
public:
    SfntOffsetTable(
        streams::Stream& stream,
        std::uint32_t fontIdx);

    std::uint32_t getTableOffset(std::uint32_t tag) const;
private:
    struct TableRecord {
        std::uint32_t tag;
        std::uint32_t offset;
    };

    std::vector<TableRecord> tableRecords;
};
