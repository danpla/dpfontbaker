
#pragma once

#include <cstdint>
#include <stdexcept>
#include <vector>
#include <memory>

#include "cp_range.h"
#include "font_renderer/font_renderer.h"
#include "geometry.h"
#include "image.h"
#include "sfnt.h"
#include "streams/const_mem_stream.h"


enum class KerningSource {
    none,
    kern,
    gpos,
    kernAndGpos
};


struct FontBakingOptions {
    std::string fontPath;
    std::string fontRenderer;
    int fontIndex;
    int fontPxSize;
    bool fontLightHinting;
    int imageMaxSize;
    Edge imagePadding;
    Edge glyphPaddingInner;
    Edge glyphPaddingOuter;
    Point glyphSpacing;
    KerningSource kerningSource;
};


/**
 * Font name
 *
 * groupFamily is used by applications that can only work with font
 * families that have no more than 4 styles (regular, italic, bold,
 * and bold italic). If the font family has no more than 4 styles,
 * groupFamily is the same as typographic family. groupFamily is
 * normally used with StyleFlags.
 *
 * For example, for DejaVu Sans Condensed Bold Oblique, family is
 * "DejaVu Sans", style is "Condensed Bold Oblique", and groupFamily
 * is "DejaVu Sans Condensed" with both StyleFlags set to true.
 */
struct FontName {
    std::string family;
    std::string style;
    std::string groupFamily;
};


/**
 * Style flags
 *
 * The style flags are intended to be used with FontName.groupFamily.
 */
struct StyleFlags {
    bool bold;
    bool italic;
};


struct Page {
    Size size;
    std::vector<std::uint_least32_t> glyphIndices;
};


struct Glyph {
    char32_t cp;
    GlyphIndex glyphIdx;
    Size size;
    Point drawOffset;
    int advance;
    std::uint_least32_t pageIdx;
    Point pagePos;
};


struct KerningPair {
    char32_t cp1;
    char32_t cp2;
    int amount;
};


class FontError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};


class Font {
public:
    Font(
        const FontBakingOptions& options,
        const cp_range::CpRangeList& cpRangeList);

    const FontBakingOptions& getBakingOptions() const;
    StyleFlags getStyleFlags() const;
    const FontName& getFontName() const;
    FontMetrics getFontMetrics() const;

    const std::vector<Page>& getPages() const;
    const std::vector<Glyph>& getGlyphs() const;
    const std::vector<KerningPair>& getKerningPairs() const;

    void renderGlyph(
        GlyphIndex glyphIdx, Image& image) const;
private:
    enum class GlyphsOrder {
        unsorted,
        sizeDescending,
        cp,
        glyphIdx
    };

    FontBakingOptions bakingOptions;

    std::vector<std::uint8_t> fontData;
    streams::ConstMemStream fontStream;

    SfntOffsetTable sfntOffsetTable;
    std::unique_ptr<FontRenderer> renderer;

    struct Head {
        enum {
            macStyleBold = 1 << 0,
            macStyleItalic = 1 << 1
        };

        std::uint16_t unitsPerEm;
        std::uint16_t macStyle;
    };

    Head head;

    struct Os2 {
        enum {
            fsSelectionItalic = 1 << 0,
            fsSelectionBold = 1 << 5,
            fsSelectionOblique = 1 << 9
        };

        std::uint16_t fsSelection;
    };

    bool hasOs2;
    Os2 os2;

    FontName fontName;

    std::vector<Page> pages;
    GlyphsOrder glyphsOrder;
    std::vector<Glyph> glyphs;
    std::vector<KerningPair> kerningPairs;

    void validateBakingOptions() const;
    void uploadFontData();

    void sortGlyphs(GlyphsOrder newOrder);
    void uploadGlyphs(const cp_range::CpRangeList& cpRangeList);
    void packGlyphs();

    void readHead();
    void readOs2();

    void readFontName();

    char32_t glyphIdxToCp(GlyphIndex glyphIdx) const;
    void readKerningPairs();
};
