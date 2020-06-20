
#include "catch.hpp"

#include <cerrno>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include "sfnt.h"
#include "str.h"
#include "streams/file_stream.h"
#include "utils.h"


namespace dpfb {


TEST_CASE("sfntTagToStr", "[sfnt]") {
    REQUIRE(
        std::strcmp(
            sfntTagToStr(sfntTag('O', 'S', '/', '2')), "OS/2") == 0);
}


static std::vector<std::string> loadCollectionListFp(std::FILE* fp)
{
    std::vector<std::string> result;

    char line[128];
    std::size_t lineNum = 0;

    while (std::fgets(line, sizeof(line), fp)) {
        ++lineNum;

        char* name;
        char* value;
        splitKeyValue(line, name, value);
        (void)value;

        if (!*name)
            continue;

        result.push_back(name);
    }

    return result;
}


static std::vector<std::string> loadCollectionList()
{
    const char* fileName = "data/collection_list.txt";
    auto* fp = std::fopen(fileName, "r");
    if (!fp)
        throw std::runtime_error(str::format(
            "Can't open %s for reading: %s",
            fileName, std::strerror(errno)));

    std::vector<std::string> result;
    try {
        result = loadCollectionListFp(fp);
    } catch (std::runtime_error&) {
        std::fclose(fp);
        throw;
    }

    std::fclose(fp);
    return result;
}


struct TableOffset {
    std::uint32_t tag;
    std::uint32_t offset;
};


static std::vector<TableOffset> loadTableOffsetsFp(std::FILE* fp)
{
    std::vector<TableOffset> result;

    char line[128];
    std::size_t lineNum = 0;

    while (std::fgets(line, sizeof(line), fp)) {
        ++lineNum;

        char* tagStr;
        char* offsetStr;
        splitKeyValue(line, tagStr, offsetStr);
        if (!*tagStr)
            continue;

        const auto tagStrLen = std::strlen(tagStr);
        if (tagStrLen < 3 || tagStrLen > 4)
            throw std::runtime_error(str::format(
                "Line %zu: Invalid tag", lineNum));

        const auto offset = std::atoi(offsetStr);
        if (offset < 0)
            throw std::runtime_error(str::format(
                "Line %zu: Offset should be positive", lineNum));

        result.push_back({
            sfntTag(
                tagStr[0],
                tagStr[1],
                tagStr[2],
                tagStrLen == 3 ? ' ' : tagStr[3]),
            static_cast<std::uint32_t>(offset)
        });
    }

    return result;
}


TEST_CASE("SfntOffsetTable", "[sfnt]") {
    using namespace streams;

    const auto collectionList = loadCollectionList();
    for (const auto& collectionName : collectionList) {
        const auto collectionFileName = "data/" + collectionName;
        INFO(collectionFileName);

        FileStream fontStream(collectionFileName, "rb");

        int fontIdx = 0;
        for (; fontIdx < 100; ++fontIdx) {
            const auto offsetsFileName = str::format(
                "%s_%02i.txt", collectionFileName.c_str(), fontIdx);

            auto* fp = std::fopen(offsetsFileName.c_str(), "r");
            if (!fp) {
                if (errno == ENOENT)
                    break;

                FAIL(str::format(
                    "Can't open %s for reading: %s",
                    offsetsFileName.c_str(), std::strerror(errno)));
            }

            INFO(offsetsFileName);
            INFO("Font index " << fontIdx);

            std::vector<TableOffset> tableOffsets;
            try {
                tableOffsets = loadTableOffsetsFp(fp);
            } catch (std::runtime_error&) {
                std::fclose(fp);
                throw;
            }
            std::fclose(fp);

            SfntOffsetTable offsetTable(fontStream, fontIdx);
            for (const auto& tableOffset : tableOffsets) {
                INFO("Tag " << sfntTagToStr(tableOffset.tag));
                REQUIRE(
                    offsetTable.getTableOffset(tableOffset.tag)
                    == tableOffset.offset);
            }
        }

        REQUIRE_THROWS_AS(
            SfntOffsetTable(fontStream, fontIdx), StreamError);
    }
}


}
