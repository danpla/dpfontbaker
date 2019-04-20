
#include <cinttypes>
#include <cstddef>

#include "font_writer/font_writer.h"
#include "font.h"
#include "str.h"


class BMFontWriter : public FontWriter {
public:
    BMFontWriter();

    const char* getDescription() const override;

    void write(
        streams::Stream& stream,
        const Font& font,
        const ImageNameFormatter& imageNameFormatter) const override;
};


BMFontWriter::BMFontWriter()
    : FontWriter("bmfont", ".fnt")
{

}


const char* BMFontWriter::getDescription() const
{
    return "BMFont text (http://www.angelcode.com/products/bmfont/)";
}


void BMFontWriter::write(
    streams::Stream& stream,
    const Font& font,
    const ImageNameFormatter& imageNameFormatter) const
{
    // Specification:
    //   http://www.angelcode.com/products/bmfont/doc/file_format.html

    const auto& bakingOptions = font.getBakingOptions();

    stream.writeStr(str::format(
        "info face=\"%s\" size=%i bold=%i italic=%i "
        "charset="" unicode=1 stretchH=100 smooth=1 aa=1 "
        "padding=%i,%i,%i,%i spacing=%i,%i outline=0\n",
        font.getFontName().groupFamily.c_str(),
        bakingOptions.fontPxSize,
        font.getStyleFlags().bold,
        font.getStyleFlags().italic,
        bakingOptions.glyphPaddingOuter.top,
        bakingOptions.glyphPaddingOuter.right,
        bakingOptions.glyphPaddingOuter.bottom,
        bakingOptions.glyphPaddingOuter.left,
        bakingOptions.glyphSpacing.x, bakingOptions.glyphSpacing.y));

    const auto metrics = font.getFontMetrics();
    const auto& pages = font.getPages();
    stream.writeStr(str::format(
        "common lineHeight=%i base=%i scaleW=%i scaleH=%i "
        "pages=%zu "
        "packed=0 alphaChnl=0 redChnl=4 greenChnl=4 blueChnl=4\n",
        metrics.lineHeight, metrics.ascender,
        bakingOptions.imageMaxSize, bakingOptions.imageMaxSize,
        pages.size()));

    for (std::size_t i = 0; i < pages.size(); ++i)
        stream.writeStr(str::format(
            "page id=%zu file=\"%s\"\n",
            i, imageNameFormatter.getImageName(i).c_str()));

    const auto& glyphs = font.getGlyphs();
    stream.writeStr(str::format("chars count=%zu\n", glyphs.size()));
    for (const auto& glyph : glyphs)
        stream.writeStr(str::format(
            "char id=%" PRIuLEAST32 " "
            "x=%i y=%i width=%i height=%i "
            "xoffset=%i yoffset=%i xadvance=%i "
            "page=%" PRIuLEAST32 " "
            "chnl=15\n",
            glyph.cp,
            glyph.pagePos.x, glyph.pagePos.y, glyph.size.w, glyph.size.h,
            glyph.drawOffset.x, glyph.drawOffset.y, glyph.advance,
            glyph.pageIdx));

    const auto& kerningPairs = font.getKerningPairs();
    if (!kerningPairs.empty()) {
        stream.writeStr(str::format
            ("kernings count=%zu\n", kerningPairs.size()));

        for (const auto& kp : kerningPairs)
            stream.writeStr(str::format(
                "kerning "
                "first=%" PRIuLEAST32 " "
                "second=%" PRIuLEAST32 " "
                "amount=%i\n",
                kp.cp1, kp.cp2, kp.amount));
    }
}


static BMFontWriter instance;
