
#include "kerning.h"

#include <algorithm>
#include <cinttypes>
#include <cmath>
#include <utility>

#include "str.h"


static float getScale(const KerningParams& params)
{
    return static_cast<float>(params.pxSize) / params.pxPerEm;
}


using namespace streams;


// https://www.microsoft.com/typography/otspec/kern.htm
std::vector<RawKerningPair> readKerningPairsKern(
    Stream& stream,
    const SfntOffsetTable& sfntOffsetTable,
    const KerningParams& params)
{
    const auto tableOffset = sfntOffsetTable.getTableOffset(
        sfntTag('k', 'e', 'r', 'n'));
    if (tableOffset == 0)
        return {};

    stream.seek(tableOffset, SeekOrigin::set);

    const auto version = stream.readU16Be();
    if (version != 0)
        // Unsupported kern table version
        return {};

    auto numTables = stream.readU16Be();
    if (numTables == 0)
        throw streams::StreamError("\"kern\" table has no subtables");

    const auto scale = getScale(params);

    std::vector<RawKerningPair> result;

    auto nextPos = stream.getPosition();
    while (numTables--) {
        // Skip subtable version
        stream.seek(sizeof(std::uint16_t), SeekOrigin::cur);
        const auto subTableLenght = stream.readU16Be();
        const auto coverage = stream.readU16Be();

        nextPos += subTableLenght;

        // https://www.microsoft.com/typography/otspec/recom.htm says:
        //
        //   ["kern" table] Should contain a single kerning pair
        //   subtable (format 0). Windows will not support format 2
        //   (two-dimensional array of kern values by class); nor
        //   multiple tables (only the first format 0 table found
        //   will be used) nor coverage bits 0 through 4 (i.e.
        //   assumes horizontal data, kerning values, no cross
        //   stream, and override).
        if (
            // Table contains vertical kerning
            !(coverage & 1)
            // Table contains minimum values instead of kerning
            || (coverage & (1 << 1))
            // Kerning is perpendicular to the flow of the text
            || (coverage & (1 << 2))
            // Only format 0 is supported
                || (coverage >> 8) != 0) {
            stream.seek(nextPos, SeekOrigin::set);
            continue;
        }

        auto numPairs = stream.readU16Be();
        // Skip binary search stuff
        stream.seek(3 * sizeof(std::uint16_t), SeekOrigin::cur);

        std::uint16_t prevGlyphIdx1 = 0;
        std::uint16_t prevGlyphIdx2 = 0;

        result.reserve(result.size() + numPairs);
        while (numPairs--) {
            const auto glyphIdx1 = stream.readU16Be();
            const auto glyphIdx2 = stream.readU16Be();
            const auto amount = stream.readS16Be();

            // Some Windows' fonts have duplicate pairs
            if (glyphIdx1 == prevGlyphIdx1
                    && glyphIdx2 == prevGlyphIdx2)
                continue;
            else {
                prevGlyphIdx1 = glyphIdx1;
                prevGlyphIdx2 = glyphIdx2;
            }

            const int scaledAmount = std::lround(amount * scale);
            if (scaledAmount == 0)
                continue;

            result.push_back({glyphIdx1, glyphIdx2, scaledAmount});
        }
    }

    return result;
}


static std::vector<std::uint16_t> readCovarageTable(Stream& stream)
{
    std::vector<std::uint16_t> result;

    const auto coverageFormat = stream.readU16Be();
    if (coverageFormat == 1) {
        auto glyphCount = stream.readU16Be();
        result.reserve(glyphCount);
        while (glyphCount--)
            result.push_back(stream.readU16Be());
    } else if (coverageFormat == 2) {
        auto rangeCount = stream.readU16Be();
        while (rangeCount--) {
            const auto startGlyphId = stream.readU16Be();
            const auto endGlyphId = stream.readU16Be();
            // Skip startCoverageIndex
            stream.seek(
                sizeof(std::uint16_t), SeekOrigin::cur);

            if (startGlyphId > endGlyphId)
                throw StreamError(str::format(
                    "Coverage table format 2 range start id "
                    "(%" PRIu16 ") > end id (%" PRIu16 ")",
                    startGlyphId,
                    endGlyphId));

            result.reserve(
                result.size() + endGlyphId - startGlyphId + 1);
            for (auto i = startGlyphId; i <= endGlyphId; ++ i)
                result.push_back(i);
        }
    } else
        throw StreamError(str::format(
            "Unknown format of coverage table: %" PRIu16,
            coverageFormat));

    return result;
}


using GlyphClasses = std::vector<std::vector<std::uint16_t>>;


static GlyphClasses readClassDefTable(
    Stream& stream, std::uint16_t classCount)
{
    GlyphClasses result;
    result.resize(classCount);

    const auto classFormat = stream.readU16Be();
    if (classFormat == 1) {
        const auto startGlyphId = stream.readU16Be();
        const auto glyphCount = stream.readU16Be();
        for (std::uint16_t i = 0; i < glyphCount; ++i) {
            const auto glyphId = startGlyphId + i;
            const auto glyphClass = stream.readU16Be();
            if (glyphClass >= classCount)
                throw StreamError(str::format(
                    "Glyph class index (%" PRIu16 ") in class "
                    "definition table format 1 exceeds the number of "
                    "classes (%" PRIu16 ")",
                    glyphClass,
                    classCount));

            result[glyphClass].push_back(glyphId);
        }
    } else if (classFormat == 2) {
        auto classRangeCount = stream.readU16Be();
        while (classRangeCount--) {
            const auto startGlyphId = stream.readU16Be();
            const auto endGlyphId = stream.readU16Be();
            if (startGlyphId > endGlyphId)
                throw StreamError(str::format(
                    "Class definition table format 2 range start id "
                    "(%" PRIu16 ") > end id (%" PRIu16 ")",
                    startGlyphId,
                    endGlyphId));

            const auto glyphClass = stream.readU16Be();
            if (glyphClass >= classCount)
                throw StreamError(str::format(
                    "Glyph class index (%" PRIu16 ") in class "
                    "definition table format 2 exceeds the number of "
                    "classes (%" PRIu16 ")",
                    glyphClass,
                    classCount));

            auto& classGlyphs = result[glyphClass];
            for (std::uint16_t i = startGlyphId; i <= endGlyphId; ++i)
                classGlyphs.push_back(i);
        }
    } else
        throw StreamError(str::format(
            "Unknown format of class definition table: %" PRIu16,
            classFormat));

    return result;
}


struct LookupContext {
    int pxSize;
    float scale;
    std::vector<RawKerningPair> kerningPairs;
};


enum GposLookup {
    gposLookupPairAdjustment = 2,
    gposLookupExtension = 9,
};


enum ValueFormat {
    xPlacement = 0x0001,
    yPlacement = 0x0002,
    xAdvance = 0x0004,
    yAdvance = 0x0008,
    xPlacementDeviceOffset = 0x0010,
    yPlacementDeviceOffset = 0x0020,
    xAdvanceDeviceOffset = 0x0040,
    yAdvanceDeviceOffset = 0x0080,
};


static bool hasKerning(int valueFormat1, int valueFormat2)
{
    return (
        (valueFormat1 & ValueFormat::xAdvance)
        && !(valueFormat2 & ValueFormat::xAdvance));
}


static int readDeviceAdjsutment(Stream& stream, int pxSize)
{
    const auto startSize = stream.readU16Be();
    const auto endSize = stream.readU16Be();
    if (startSize > endSize)
        throw StreamError(str::format(
            "Device table start size (%" PRIu16 ") > "
            "end size (%" PRIu16 ")",
            startSize,
            endSize));

    if (pxSize < startSize || pxSize > endSize)
        return 0;

    const auto deltaFormat = stream.readU16Be();
    if (deltaFormat < 1 || deltaFormat > 3)
        return 0;

    const auto valueBits = 1 << deltaFormat;
    const auto valuesPerU16 = 16 / valueBits;
    const auto valueIdx = pxSize - startSize;
    const auto valueU16Idx = valueIdx / valuesPerU16;
    stream.seek(valueU16Idx * sizeof(std::uint16_t), SeekOrigin::cur);

    const auto u16 = stream.readU16Be();
    const auto u16rshift = (
        ((valueU16Idx + 1) * valuesPerU16 - 1 - valueIdx) * valueBits);
    const auto mask = 0xff >> (8 - valueBits);

    int value = (u16 >> u16rshift) & mask;
    if (value >= (mask + 1) >> 1)
        value -= mask + 1;

    return value;
}


enum ValueIdx {
    valueIdxXPlacement,
    valueIdxYPlacement,
    valueIdxXAdvance,
    valueIdxYAdvance,
    numValues
};


static void readValuesForSize(
    Stream& stream,
    std::uint32_t subTablePos,
    const LookupContext& ctx,
    int values[numValues],
    int valueFormat)
{
    for (int i = 0; i < numValues; ++i)
        if (valueFormat & (1 << i))
            values[i] = std::lround(stream.readS16Be() * ctx.scale);
        else
            values[i] = 0;

    std::uint16_t deviceOffsets[numValues];
    for (int i = 0; i < numValues; ++i)
        if (valueFormat & (1 << (i + numValues)))
            deviceOffsets[i] = stream.readU16Be();
        else
            deviceOffsets[i] = 0;

    const auto prevPos = stream.getPosition();
    for (int i = 0; i < numValues; ++i) {
        const auto deviceOffset = deviceOffsets[i];
        if (deviceOffset == 0)
            continue;

        stream.seek(subTablePos + deviceOffset, SeekOrigin::set);
        values[i] += readDeviceAdjsutment(stream, ctx.pxSize);
    }
    stream.seek(prevPos, SeekOrigin::set);
}


static void readGposPairAdjustmentFormat1(Stream& stream, LookupContext& ctx)
{
    const auto subTablePos = (
        stream.getPosition()
        // Format
        - sizeof(std::uint16_t));

    const auto coverageOffset = stream.readU16Be();
    const auto valueFormat1 = stream.readU16Be();
    const auto valueFormat2 = stream.readU16Be();

    if (!hasKerning(valueFormat1, valueFormat2))
        return;

    const auto pairSetCount = stream.readU16Be();

    const auto pairSetsPos = stream.getPosition();

    stream.seek(subTablePos + coverageOffset, SeekOrigin::set);
    const auto coverage = readCovarageTable(stream);

    if (pairSetCount != coverage.size())
        throw StreamError(str::format(
            "\"GPOS\" pair adjustment table format 1 "
            "pairSetCount (%" PRIu16 ") doesn't match the number of "
            "glyphs in coverage table (%zu)",
            pairSetCount,
            coverage.size()));

    stream.seek(pairSetsPos, SeekOrigin::set);
    for (std::uint16_t i = 0; i < pairSetCount; ++i) {
        const auto glyphIdx1 = coverage[i];

        const auto pairSetOffset = stream.readU16Be();

        const auto pos = stream.getPosition();
        stream.seek(subTablePos + pairSetOffset, SeekOrigin::set);

        auto pairValueCount = stream.readU16Be();
        while (pairValueCount--) {
            const auto glyphIdx2 = stream.readU16Be();

            int values1[numValues];
            readValuesForSize(stream, subTablePos, ctx, values1, valueFormat1);
            int values2[numValues];
            readValuesForSize(stream, subTablePos, ctx, values2, valueFormat2);
            (void)values2;
            if (values1[valueIdxXAdvance] == 0)
                continue;

            ctx.kerningPairs.push_back(
                {glyphIdx1, glyphIdx2, values1[valueIdxXAdvance]});
        }

        stream.seek(pos, SeekOrigin::set);
    }
}


static std::uint16_t getGlyphClass(
    std::uint16_t glyphIdx, const GlyphClasses& classes)
{
    if (classes.size() < 2)
        return 0;

    for (std::uint16_t i = 1; i < classes.size(); ++i) {
        const auto& glyphIndices = classes[i];
        if (glyphIdx < glyphIndices.front()
                || glyphIdx > glyphIndices.back())
            continue;

        const auto iter = std::lower_bound(
            glyphIndices.begin(),
            glyphIndices.end(),
            glyphIdx);
        if (iter != glyphIndices.end() && *iter == glyphIdx)
            return i;
    }

    return 0;
}


static void readGposPairAdjustmentFormat2(Stream& stream, LookupContext& ctx)
{
    const auto subTablePos = (
        stream.getPosition()
        // Format
        - sizeof(std::uint16_t));

    const auto coverageOffset = stream.readU16Be();
    const auto valueFormat1 = stream.readU16Be();
    const auto valueFormat2 = stream.readU16Be();

    if (!hasKerning(valueFormat1, valueFormat2))
        return;

    const auto classDef1Offset = stream.readU16Be();
    const auto classDef2Offset = stream.readU16Be();
    const auto class1Count = stream.readU16Be();
    if (class1Count == 0)
        throw StreamError(
            "\"GPOS\" pair adjustment format 2 class 1 count is 0");
    const auto class2Count = stream.readU16Be();
    if (class2Count == 0)
        throw StreamError(
            "\"GPOS\" pair adjustment format 2 class 2 count is 0");

    const auto valuesPos = stream.getPosition();

    stream.seek(subTablePos + coverageOffset, SeekOrigin::set);
    const auto coverage = readCovarageTable(stream);

    stream.seek(subTablePos + classDef1Offset, SeekOrigin::set);
    auto class1 = readClassDefTable(stream, class1Count);
    stream.seek(subTablePos + classDef2Offset, SeekOrigin::set);
    const auto class2 = readClassDefTable(stream, class2Count);

    // Glyphs not assigned to a class go to class 0.
    for (auto glyphIdx : coverage) {
        const auto glyphClass = getGlyphClass(glyphIdx, class1);
        if (glyphClass == 0)
            class1[0].push_back(glyphIdx);
    }

    stream.seek(valuesPos, SeekOrigin::set);
    for (std::uint16_t ci1 = 0; ci1 < class1Count; ++ci1) {
        for (std::uint16_t ci2 = 0; ci2 < class2Count; ++ci2) {
            int values1[numValues];
            readValuesForSize(stream, subTablePos, ctx, values1, valueFormat1);
            int values2[numValues];
            readValuesForSize(stream, subTablePos, ctx, values2, valueFormat2);
            (void)values2;
            if (values1[valueIdxXAdvance] == 0)
                continue;

            ctx.kerningPairs.reserve(
                ctx.kerningPairs.size()
                + class1[ci1].size() * class2[ci2].size());
            for (auto glyphIdx1 : class1[ci1])
                for (auto glyphIdx2 : class2[ci2])
                    ctx.kerningPairs.push_back(
                        {glyphIdx1,
                            glyphIdx2,
                            values1[valueIdxXAdvance]});
        }
    }
}


static void readGposPairAdjustment(Stream& stream, LookupContext& ctx)
{
    const auto posFormat = stream.readU16Be();
    if (posFormat == 1)
        readGposPairAdjustmentFormat1(stream, ctx);
    else if (posFormat == 2)
        readGposPairAdjustmentFormat2(stream, ctx);
    else
        throw StreamError(str::format(
            "Unknown format of \"GPOS\" pair adjustment subtable: "
            "%" PRIu16,
            posFormat));
}


static void readGposSubtable(
    Stream& stream, LookupContext& ctx, int lookupType);


static void readGposExtension(Stream& stream, LookupContext& ctx)
{
    const auto subTableOffset = stream.getPosition();

    const auto posFormat = stream.readU16Be();
    if (posFormat != 1)
        throw StreamError(str::format(
            "Unknown format of \"GPOS\" extension subtable: %" PRIu16,
            posFormat));

    const auto extensionLookupType = stream.readU16Be();
    if (extensionLookupType == gposLookupExtension)
        throw StreamError(
            "\"GPOS\" extension subtables cannot be nested");

    stream.seek(
        subTableOffset + stream.readU32Be(), SeekOrigin::set);
    readGposSubtable(stream, ctx, extensionLookupType);
}


static void readGposSubtable(
    Stream& stream, LookupContext& ctx, int lookupType)
{
    if (lookupType == gposLookupPairAdjustment)
        readGposPairAdjustment(stream, ctx);
    else if (lookupType == gposLookupExtension)
        readGposExtension(stream, ctx);
}


static void appendFeatureTable(
    Stream& stream,
    std::vector<std::uint16_t>& lookupIndices,
    bool ignoreDuplicates = true)
{
    // Skip featureParams
    stream.seek(sizeof(std::uint16_t), SeekOrigin::cur);

    auto lookupIdxCount = stream.readU16Be();
    if (!ignoreDuplicates)
        lookupIndices.reserve(lookupIndices.size() + lookupIdxCount);
    while (lookupIdxCount--) {
        const auto lookupIdx = stream.readU16Be();
        if (ignoreDuplicates
                && std::find(
                    lookupIndices.begin(),
                    lookupIndices.end(),
                    lookupIdx) != lookupIndices.end())
            continue;

        lookupIndices.push_back(lookupIdx);
    }
}


static std::vector<std::uint16_t> getAllKernFeatures(
    Stream& stream,
    std::uint32_t featureListPos)
{
    std::vector<std::uint16_t> lookupIndices;

    stream.seek(featureListPos, SeekOrigin::set);
    auto featureCount = stream.readU16Be();
    while (featureCount--) {
        const auto featureTag = stream.readU32Be();
        const auto featureOffset = stream.readU16Be();
        if (featureTag != sfntTag('k', 'e', 'r', 'n'))
            continue;

        const auto prevPos = stream.getPosition();

        stream.seek(featureListPos + featureOffset, SeekOrigin::set);
        // There can be duplicate lookup indices, since some
        // applications don't optimize the feature list, creating
        // a separate set of feature records for every language
        // (including default). For example, if we have "cyrl"
        // script with BGR, SRB, and default language, there may
        // be 4 separate "kern" feature records with the same
        // feature table.
        appendFeatureTable(stream, lookupIndices, true);

        stream.seek(prevPos, SeekOrigin::set);
    }

    return lookupIndices;
}


static void readLookupTable(Stream& stream, LookupContext& ctx)
{
    const auto lookupTablePos = stream.getPosition();

    const auto lookupType = stream.readU16Be();
    if (lookupType != gposLookupPairAdjustment
            && lookupType != gposLookupExtension)
        return;

    // Skip lookupFlag
    stream.seek(sizeof(std::uint16_t), SeekOrigin::cur);

    auto subTableCount = stream.readU16Be();
    while (subTableCount--) {
        const auto subTableOffset = stream.readU16Be();
        const auto prevPos = stream.getPosition();

        stream.seek(
            lookupTablePos + subTableOffset, SeekOrigin::set);
        readGposSubtable(stream, ctx, lookupType);

        stream.seek(prevPos, SeekOrigin::set);
    }
}


static void lookupFeatures(
    Stream& stream,
    std::uint32_t lookupListPos,
    LookupContext& ctx,
    const std::vector<std::uint16_t>& lookupIndices)
{
    stream.seek(lookupListPos, SeekOrigin::set);

    const auto lookupCount = stream.readU16Be();

    for (const auto lookupIdx : lookupIndices) {
        if (lookupIdx >= lookupCount)
            throw StreamError(str::format(
                "Lookup list lookupIdx (%" PRIu16 ") >= "
                "lookupCount (%" PRIu16 ")",
                lookupIdx,
                lookupCount));

        stream.seek(
            lookupListPos
            + sizeof(std::uint16_t)  // lookupCount
            + sizeof(std::uint16_t) * lookupIdx,
            SeekOrigin::set);
        const auto lookupOffset = stream.readU16Be();

        stream.seek(lookupListPos + lookupOffset, SeekOrigin::set);
        readLookupTable(stream, ctx);
    }
}


std::vector<RawKerningPair> readKerningPairsGpos(
    Stream& stream,
    const SfntOffsetTable& sfntOffsetTable,
    const KerningParams& params)
{
    const auto tableOffset = sfntOffsetTable.getTableOffset(
        sfntTag('G', 'P', 'O', 'S'));
    if (tableOffset == 0)
        return {};

    stream.seek(tableOffset, SeekOrigin::set);

    const auto versionMajor = stream.readU16Be();
    if (versionMajor != 1)
        throw StreamError(str::format(
            "Unsupported \"GPOS\" major version %" PRIu16,
            versionMajor));
    // Skip minor version
    stream.seek(sizeof(std::uint16_t), SeekOrigin::cur);

    // Skip scriptListOffset
    stream.seek(sizeof(std::uint16_t), SeekOrigin::cur);
    const auto featureListOffset = stream.readU16Be();
    const auto lookupListOffset = stream.readU16Be();

    const auto lookupIndices = getAllKernFeatures(
        stream, tableOffset + featureListOffset);

    LookupContext ctx;
    ctx.pxSize = params.pxSize;
    ctx.scale = getScale(params);
    lookupFeatures(
        stream, tableOffset + lookupListOffset, ctx, lookupIndices);

    return std::move(ctx.kerningPairs);
}
