
#include <cinttypes>
#include <cstddef>

#include "font_writer/font_writer.h"
#include "font.h"
#include "str.h"


// To validate a json file:
//   python3 -m json.tool *.json > /dev/null
// Alternatively, a good online validator:
//   https://www.freeformatter.com/json-validator.html


class JsonFontWriter : public dpfb::FontWriter {
public:
    JsonFontWriter();

    const char* getDescription() const override;

    void write(
        dpfb::streams::Stream& stream,
        const dpfb::Font& font,
        const dpfb::ImageNameFormatter& imageNameFormatter) const override;
};


JsonFontWriter::JsonFontWriter()
    : FontWriter("json", ".json")
{

}


const char* JsonFontWriter::getDescription() const
{
    return "Generic JSON";
}


static const char* jsonBool(bool v)
{
    return v ? "true" : "false";
}


void JsonFontWriter::write(
    dpfb::streams::Stream& stream,
    const dpfb::Font& font,
    const dpfb::ImageNameFormatter& imageNameFormatter) const
{
    stream.writeStr("{\n");

    const auto& name = font.getFontName();
    stream.writeStr(dpfb::str::format(
        "  \"name\": {\n"
        "    \"family\": \"%s\",\n"
        "    \"style\": \"%s\",\n"
        "    \"groupFamily\": \"%s\"\n"
        "  },\n",
        name.family.c_str(),
        name.style.c_str(),
        name.groupFamily.c_str()));

    const auto styleFlags = font.getStyleFlags();
    stream.writeStr(dpfb::str::format(
        "  \"styleFlags\": {\n"
        "    \"bold\": %s,\n"
        "    \"italic\": %s\n"
        "  },\n",
        jsonBool(styleFlags.bold),
        jsonBool(styleFlags.italic)));

    const auto metrics = font.getFontMetrics();
    stream.writeStr(dpfb::str::format(
        "  \"metrics\": {\n"
        "    \"ascender\": %i,\n"
        "    \"descender\": %i,\n"
        "    \"lineHeight\": %i\n"
        "  },\n",
        metrics.ascender,
        metrics.descender,
        metrics.lineHeight));

    const auto& bakingOptions = font.getBakingOptions();
    stream.writeStr(dpfb::str::format(
        "  \"bakingOptions\": {\n"
        "    \"fontPxSize\": %i,\n"
        "    \"imageMaxSize\": %i,\n"
        "    \"imagePadding\": {\n"
        "      \"top\": %i,\n"
        "      \"bottom\": %i,\n"
        "      \"left\": %i,\n"
        "      \"right\": %i\n"
        "    },\n"
        "    \"glyphPaddingInner\": {\n"
        "      \"top\": %i,\n"
        "      \"bottom\": %i,\n"
        "      \"left\": %i,\n"
        "      \"right\": %i\n"
        "    },\n"
        "    \"glyphPaddingOuter\": {\n"
        "      \"top\": %i,\n"
        "      \"bottom\": %i,\n"
        "      \"left\": %i,\n"
        "      \"right\": %i\n"
        "    },\n"
        "    \"glyphSpacing\": {\n"
        "      \"x\": %i,\n"
        "      \"y\": %i\n"
        "    }\n"
        "  },\n",
        bakingOptions.fontPxSize,
        bakingOptions.imageMaxSize,
        bakingOptions.imagePadding.top,
        bakingOptions.imagePadding.bottom,
        bakingOptions.imagePadding.left,
        bakingOptions.imagePadding.right,
        bakingOptions.glyphPaddingInner.top,
        bakingOptions.glyphPaddingInner.bottom,
        bakingOptions.glyphPaddingInner.left,
        bakingOptions.glyphPaddingInner.right,
        bakingOptions.glyphPaddingOuter.top,
        bakingOptions.glyphPaddingOuter.bottom,
        bakingOptions.glyphPaddingOuter.left,
        bakingOptions.glyphPaddingOuter.right,
        bakingOptions.glyphSpacing.x,
        bakingOptions.glyphSpacing.y));

    stream.writeStr("  \"pages\": [\n");

    const auto& pages = font.getPages();
    for (std::size_t i = 0; i < pages.size(); ++i) {
        const auto& page = pages[i];
        stream.writeStr(dpfb::str::format(
            "    {\n"
            "      \"name\": \"%s\",\n"
            "      \"size\": {\n"
            "        \"w\": %i,\n"
            "        \"h\": %i\n"
            "      }\n"
            "    }%s\n",
            imageNameFormatter.getImageName(i).c_str(),
            page.size.w,
            page.size.h,
            i + 1 != pages.size() ? "," : ""));
    }

    stream.writeStr("  ],\n");

    stream.writeStr("  \"glyphs\": [\n");

    const auto& glyphs = font.getGlyphs();
    for (std::size_t i = 0; i < glyphs.size(); ++i) {
        const auto& glyph = glyphs[i];
        stream.writeStr(dpfb::str::format(
            "    {\n"
            "      \"codePoint\": %" PRIuLEAST32 ",\n"
            "      \"size\": {\n"
            "        \"w\": %i,\n"
            "        \"h\": %i\n"
            "      },\n"
            "      \"drawOffset\": {\n"
            "        \"x\": %i,\n"
            "        \"y\": %i\n"
            "      },\n"
            "      \"advance\": %i,\n"
            "      \"pageIndex\": %" PRIuLEAST32 ",\n"
            "      \"pagePos\": {\n"
            "        \"x\": %i,\n"
            "        \"y\": %i\n"
            "      }\n"
            "    }%s\n",
            glyph.cp,
            glyph.size.w,
            glyph.size.h,
            glyph.drawOffset.x,
            glyph.drawOffset.y,
            glyph.advance,
            glyph.pageIdx,
            glyph.pagePos.x,
            glyph.pagePos.y,
            i + 1 != glyphs.size() ? "," : ""));
    }

    stream.writeStr("  ],\n");

    stream.writeStr("  \"kerningPairs\": [\n");

    const auto& kerningPairs = font.getKerningPairs();
    for (std::size_t i = 0; i < kerningPairs.size(); ++i) {
        const auto& kp = kerningPairs[i];
        stream.writeStr(dpfb::str::format(
            "    {\n"
            "      \"codePoint1\": %" PRIuLEAST32 ",\n"
            "      \"codePoint2\": %" PRIuLEAST32 ",\n"
            "      \"amount\": %i\n"
            "    }%s\n",
            kp.cp1,
            kp.cp2,
            kp.amount,
            i + 1 != kerningPairs.size() ? "," : ""));
    }

    stream.writeStr("  ]\n");

    stream.writeStr("}");
}


static JsonFontWriter instance;
