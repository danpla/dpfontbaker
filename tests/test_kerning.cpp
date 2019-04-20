
#include "catch.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include "font.h"
#include "kerning.h"
#include "str.h"
#include "streams/file_stream.h"
#include "font_renderer/font_renderer.h"
#include "utils.h"


struct Test {
    std::string name;
    KerningSource kerningSource;
};


static std::vector<Test> loadTestListFp(std::FILE* fp)
{
    std::vector<Test> result;

    char line[128];
    std::size_t lineNum = 0;

    while (std::fgets(line, sizeof(line), fp)) {
        ++lineNum;

        char* name;
        char* kerningSourceStr;
        splitKeyValue(line, name, kerningSourceStr);
        if (!*name)
            continue;
        if (!*kerningSourceStr)
            throw std::runtime_error(str::format(
                "Line %zu: Invalid pair format", lineNum));

        KerningSource kerningSource;
        if (std::strcmp(kerningSourceStr, "kern") == 0)
            kerningSource = KerningSource::kern;
        else if (std::strcmp(kerningSourceStr, "gpos") == 0)
            kerningSource = KerningSource::gpos;
        else if (std::strcmp(kerningSourceStr, "kernAndGpos") == 0)
            kerningSource = KerningSource::kernAndGpos;
        else
            throw std::runtime_error(str::format(
                "Line %zu: Invalid kerning source \"%s\"",
                lineNum, kerningSourceStr));

        result.push_back({name, kerningSource});
    }

    return result;
}


static std::vector<Test> loadTestList()
{
    const char* fileName = "data/kerning_list.txt";
    auto* fp = std::fopen(fileName, "r");
    if (!fp)
        throw std::runtime_error(str::format(
            "Can't open %s for reading: %s",
            fileName, std::strerror(errno)));

    std::vector<Test> result;
    try {
        result = loadTestListFp(fp);
    } catch (std::runtime_error&) {
        std::fclose(fp);
        throw;
    }

    std::fclose(fp);
    return result;
}


static std::ostream& operator<<(
    std::ostream& os, const RawKerningPair& pair) {
    return (
        os << '{'
        << pair.glyphIdx1 << ", "
        << pair.glyphIdx2 << ", "
        << pair.amount << '}');
}


static bool operator==(const RawKerningPair& a, const RawKerningPair& b)
{
    return (
        a.glyphIdx1 == b.glyphIdx1
        && a.glyphIdx2 == b.glyphIdx2
        && a.amount == b.amount);
}


struct KerningTest {
    int pxSize;
    int pxPerEm;
    std::vector<RawKerningPair> pairs;
};


static void sortKerningPairs(std::vector<RawKerningPair>& pairs)
{
    struct Cmp {
        bool operator()(
            const RawKerningPair& a, const RawKerningPair& b) const
        {
            if (a.glyphIdx1 != b.glyphIdx1)
                return a.glyphIdx1 < b.glyphIdx1;
            else if (a.glyphIdx2 != b.glyphIdx2)
                return a.glyphIdx2 < b.glyphIdx2;
            else
                return a.amount < b.amount;
        }
    };

    std::sort(pairs.begin(), pairs.end(), Cmp());
}


static void assignKeyValue(
    const char* key,
    const char* value,
    KerningTest& kerningTest,
    const FontRenderer& fontRenderer)
{
    if (std::strcmp(key, "pxSize") == 0) {
        kerningTest.pxSize = std::atoi(value);
        if (kerningTest.pxSize < 1)
            throw std::runtime_error("pxSize < 1");
    } else if (std::strcmp(key, "pxPerEm") == 0) {
        kerningTest.pxPerEm = std::atoi(value);
        if (kerningTest.pxPerEm < 1)
            throw std::runtime_error("pxPerEm < 1");
    } else if (std::strcmp(key, "pair") == 0) {
        RawKerningPair pair;
        char c1, c2;
        if (std::sscanf(
                value,
                " %c %c %d",
                &c1, &c2, &pair.amount) != 3)
            throw std::runtime_error("Can't read pair");

        pair.glyphIdx1 = fontRenderer.getGlyphIndex(c1);
        pair.glyphIdx2 = fontRenderer.getGlyphIndex(c2);

        kerningTest.pairs.push_back(pair);
    } else
        throw std::runtime_error(str::format(
            "Unknown key \"%s\"", key));
}


static KerningTest loadKerningTestFp(
    std::FILE* fp, const FontRenderer& fontRenderer)
{
    KerningTest result {-1, -1, {}};

    char line[128];
    std::size_t lineNum = 0;

    while (std::fgets(line, sizeof(line), fp)) {
        ++lineNum;

        char* key;
        char* value;
        splitKeyValue(line, key, value);

        if (!*key)
            continue;

        if (!*value)
            throw std::runtime_error(str::format(
                "Line %zu: Key \"%s\" has no value", lineNum, key));

        try {
            assignKeyValue(key, value, result, fontRenderer);
        } catch (std::runtime_error& e) {
            throw std::runtime_error(str::format(
                "Line %zu: %s", lineNum, e.what()));
        }
    }

    if (result.pxSize == -1)
        throw std::runtime_error("pxSize not found");
    if (result.pxPerEm == -1)
        throw std::runtime_error("pxPerEm not found");

    return result;
}


static std::vector<RawKerningPair> readKerningPairs(
    streams::Stream& stream,
    const SfntOffsetTable& sfntOffsetTable,
    const KerningParams& kerningParams,
    KerningSource kerningSource)
{
    std::vector<RawKerningPair> pairs;

    if (kerningSource == KerningSource::gpos
            || kerningSource == KerningSource::kernAndGpos) {
        pairs = readKerningPairsGpos(
            stream, sfntOffsetTable, kerningParams);
        if (!pairs.empty() || kerningSource == KerningSource::gpos)
            return pairs;
        // else fall back to kern
    }

    pairs = readKerningPairsKern(
        stream, sfntOffsetTable, kerningParams);

    return pairs;
}


static std::vector<std::uint8_t> getData(const char* fileName)
{
    std::vector<std::uint8_t> data;
    streams::FileStream f(fileName, "rb");
    data.resize(f.getSize());
    f.readBuffer(&data[0], data.size());
    return data;
}


TEST_CASE("Kerning", "[kerning]") {
    const auto tests = loadTestList();

    char fileName[128];
    for (const auto& test : tests) {
        std::snprintf(
            fileName, sizeof(fileName),
            "data/kerning_%s.otf", test.name.c_str());

        INFO(fileName);
        const auto fontData = getData(fileName);
        streams::ConstMemStream fontStream(
            &fontData[0], fontData.size());
        SfntOffsetTable sfntOffsetTable(fontStream, 0);

        // We run the tests with all font renderers to ensure that
        // code point to glyph index conversion is the same.
        const auto* creator = FontRendererCreator::getFirst();
        REQUIRE(creator);
        for (const auto* c = creator; c; c = c->getNext()) {
            std::unique_ptr<FontRenderer> fontRendererPtr(
                // The font size doesn't matter
                c->create({&fontData[0], fontData.size(), 12, false}));
            INFO("Font renderer " << c->getName());

            for (int i = 0; i < 100; ++i) {
                std::snprintf(
                    fileName, sizeof(fileName),
                    "data/kerning_%s_%02i.txt", test.name.c_str(), i);

                auto* fp = std::fopen(fileName, "r");
                if (!fp) {
                    if (errno == ENOENT)
                        continue;

                    FAIL(str::format(
                        "Can't open %s for reading: %s",
                        fileName, std::strerror(errno)));
                }

                INFO(fileName);

                KerningTest kerningTest;
                try {
                    kerningTest = loadKerningTestFp(
                        fp, *fontRendererPtr);
                } catch (std::runtime_error&) {
                    std::fclose(fp);
                    throw;
                }
                std::fclose(fp);

                sortKerningPairs(kerningTest.pairs);

                const KerningParams kerningParams {
                    kerningTest.pxSize,
                    kerningTest.pxPerEm
                };
                auto pairs = readKerningPairs(
                    fontStream,
                    sfntOffsetTable,
                    kerningParams,
                    test.kerningSource);
                sortKerningPairs(pairs);

                REQUIRE(kerningTest.pairs == pairs);
            }
        }
    }
}
