
#include "sfnt.h"

#include <cinttypes>

#include "str.h"


namespace dpfb {


const char* sfntTagToStr(std::uint32_t tag)
{
    static char buf[5];

    for (int i = 0; i < 4; ++i)
        buf[i] = (tag >> ((3 - i) * 8)) & 0xff;

    return buf;
}


static bool checkOffsetTableSignature(std::uint32_t signature)
{
    static const std::uint32_t signatures[] = {
        0x00010000,
        0x00020000,
        sfntTag('t', 'r', 'u', 'e'),
        sfntTag('t', 'y', 'p', '1'),
        sfntTag('O', 'T', 'T', 'O'),
    };

    for (auto sign : signatures)
        if (signature == sign)
            return true;

    return false;
}


using namespace streams;


SfntOffsetTable::SfntOffsetTable(
        Stream& stream, std::uint32_t fontIdx)
    : tableRecords {}
{
    stream.seek(0, SeekOrigin::set);

    auto version = stream.readU32Be();
    if (version == sfntTag('t', 't', 'c', 'f')) {
        const auto versionMajor = stream.readU16Be();
        if (versionMajor != 1 && versionMajor != 2)
            throw StreamError(str::format(
                "Invalid TTC header major version %" PRIu16,
                versionMajor));

        const auto versionMinor = stream.readU16Be();
        if (versionMinor != 0)
            throw StreamError(str::format(
                "Invalid TTC header minor version %" PRIu16,
                versionMinor));

        const auto numFonts = stream.readU32Be();
        if (numFonts == 0)
            throw StreamError("Collection has no fonts");

        if (fontIdx >= numFonts)
            throw StreamError(str::format(
                "Collection contains only %" PRIu32 " fonts",
                numFonts));

        stream.seek(fontIdx * sizeof(std::uint32_t), SeekOrigin::cur);
        const auto fontOffset = stream.readU32Be();
        stream.seek(fontOffset, SeekOrigin::set);

        version = stream.readU32Be();
    } else if (fontIdx > 0)
        throw StreamError(str::format(
            "Can't load font at index %" PRIu32 " because font is "
            "not a collection",
            fontIdx));

    if (!checkOffsetTableSignature(version))
        throw StreamError(str::format(
            "Unsupported font format 0x%08" PRIx32,
            version));

    const auto numTables = stream.readU16Be();
    if (numTables == 0)
        throw StreamError("Font has no tables");

    // Skip search range, entry selector, and range shift
    stream.seek(3 * sizeof(std::uint16_t), SeekOrigin::cur);

    tableRecords.reserve(numTables);
    for (std::uint16_t i = 0; i < numTables; ++i) {
        TableRecord tableRecord;
        tableRecord.tag = stream.readU32Be();

        // Skip checksum
        stream.seek(sizeof(std::uint32_t), SeekOrigin::cur);

        tableRecord.offset = stream.readU32Be();
        tableRecords.push_back(tableRecord);

        // Skip length
        stream.seek(sizeof(std::uint32_t), SeekOrigin::cur);
    }
}


std::uint32_t SfntOffsetTable::getTableOffset(std::uint32_t tag) const
{
    for (const auto& tableRecord : tableRecords)
        if (tableRecord.tag == tag)
            return tableRecord.offset;

    return 0;
}


}
