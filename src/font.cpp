
#include "font.h"

#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <cmath>

#include "dp_rect_pack.h"

#include "kerning.h"
#include "str.h"
#include "streams/file_stream.h"
#include "unicode.h"


using namespace streams;
using namespace Geometry;


static std::vector<std::uint8_t> getData(const std::string& fileName)
{
    FileStream f(fileName, "rb");
    std::vector<std::uint8_t> data;
    data.resize(f.getSize());
    f.readBuffer(&data[0], data.size());
    return data;
}


Font::Font(
        const FontBakingOptions& options,
        const cp_range::CpRangeList& cpRangeList)
    : bakingOptions {options}
    , fontData {getData(options.fontPath)}
    , fontStream {&fontData[0], fontData.size()}
    , sfntOffsetTable {
        fontStream,
        std::max<std::uint32_t>(bakingOptions.fontIndex, 0)
    }
    , renderer {}
    , head {}
    , hasOs2 {}
    , os2 {}
    , fontName {}
    , pages {}
    , glyphsOrder {}
    , glyphs {}
    , kerningPairs {}
{
    validateBakingOptions();

    const FontRendererArgs args {
        &fontData[0],
        fontData.size(),
        bakingOptions.fontPxSize,
        bakingOptions.hinting
    };

    try {
        renderer.reset(
            FontRenderer::create(
                bakingOptions.fontRenderer.c_str(), args));
    } catch (FontRendererError& e) {
        throw FontError(str::format(
            "Can't create %s font renderer: %s",
            bakingOptions.fontRenderer.c_str(), e.what()));
    }

    uploadGlyphs(cpRangeList);
    packGlyphs();

    readHead();
    readOs2();

    readFontName();

    // readKerningPairs() must be called after uploadGlyphs(), since
    // we will need to convert glyph indices back to code points.
    readKerningPairs();

    struct CmpKerningPairsByCp {
        bool operator()(const KerningPair& a, const KerningPair& b) const
        {
            if (a.cp1 != b.cp1)
                return a.cp1 < b.cp1;
            else
                return a.cp2 < b.cp2;
        }
    };
    // Stable sort because we need to keep only the first added pair
    // in case of duplicates.
    std::stable_sort(
        kerningPairs.begin(), kerningPairs.end(), CmpKerningPairsByCp());

    struct KerningPairsEqualByCp {
        bool operator()(const KerningPair& a, const KerningPair& b) const
        {
            return a.cp1 == b.cp1 && a.cp2 == b.cp2;
        }
    };
    kerningPairs.erase(
        std::unique(
            kerningPairs.begin(),
            kerningPairs.end(),
            KerningPairsEqualByCp()),
        kerningPairs.end());

    sortGlyphs(GlyphsOrder::cp);
    for (std::size_t i = 0; i < glyphs.size(); ++i)
        pages[glyphs[i].pageIdx].glyphIndices.push_back(i);
}


const FontBakingOptions& Font::getBakingOptions() const
{
    return bakingOptions;
}


StyleFlags Font::getStyleFlags() const
{
    StyleFlags flags;

    if (hasOs2) {
        flags.bold = os2.fsSelection & Os2::fsSelectionBold;
        flags.italic = (
            (os2.fsSelection & Os2::fsSelectionOblique)
            || (os2.fsSelection & Os2::fsSelectionItalic));
    } else {
        flags.bold = head.macStyle & Head::macStyleBold;
        flags.italic = head.macStyle & Head::macStyleItalic;
    }

    return flags;
}


const FontName& Font::getFontName() const
{
    return fontName;
}


FontMetrics Font::getFontMetrics() const
{
    auto metrics = renderer->getFontMetrics();

    metrics.ascender += bakingOptions.glyphPaddingInner.top;
    metrics.descender -= bakingOptions.glyphPaddingInner.bottom;
    metrics.lineHeight += (
        bakingOptions.glyphPaddingInner.top
        + bakingOptions.glyphPaddingInner.bottom);

    return metrics;
}


const std::vector<Glyph>& Font::getGlyphs() const
{
    return glyphs;
}


const std::vector<KerningPair>& Font::getKerningPairs() const
{
    return kerningPairs;
}


const std::vector<Page>& Font::getPages() const
{
    return pages;
}


void Font::renderGlyph(
    GlyphIndex glyphIdx, Image& image) const
{
    const auto paddingTop = (
        bakingOptions.glyphPaddingInner.top
        + bakingOptions.glyphPaddingOuter.top);
    const auto paddingLeft = (
        bakingOptions.glyphPaddingInner.left
        + bakingOptions.glyphPaddingOuter.left);

    const auto xPadding = (
        paddingLeft
        + bakingOptions.glyphPaddingInner.right
        + bakingOptions.glyphPaddingOuter.right);
    const auto yPadding = (
        paddingTop
        + bakingOptions.glyphPaddingInner.bottom
        + bakingOptions.glyphPaddingOuter.bottom);
    if (xPadding > image.getWidth() || yPadding > image.getHeight())
        return;

    Image adjustedImage(
        image.getData() + paddingTop * image.getPitch() + paddingLeft,
        image.getWidth() - xPadding,
        image.getHeight() - yPadding,
        image.getPitch());

    renderer->renderGlyph(glyphIdx, adjustedImage);
}


void Font::validateBakingOptions() const
{
    if (!FontRenderer::exists(
            bakingOptions.fontRenderer.c_str()))
        throw FontError(str::format(
            "No such font renderer: \"%s\"",
            bakingOptions.fontRenderer.c_str()));

    if (bakingOptions.fontIndex < 0)
        throw FontError("Font index should be > 0");

    if (bakingOptions.fontPxSize <= 0)
        throw FontError("Font size should be > 0");

    if (bakingOptions.imageMaxSize <= 0)
        throw FontError("Image max size should be > 0");

    if (bakingOptions.imagePadding.top < 0
            || bakingOptions.imagePadding.bottom < 0
            || bakingOptions.imagePadding.left < 0
            || bakingOptions.imagePadding.right < 0)
        throw FontError("Image padding should be >= 0");

    if (bakingOptions.glyphPaddingInner.top < 0
            || bakingOptions.glyphPaddingInner.bottom < 0
            || bakingOptions.glyphPaddingInner.left < 0
            || bakingOptions.glyphPaddingInner.right < 0)
        throw FontError("Glyph inner padding should be >= 0");

    if (bakingOptions.glyphPaddingOuter.top < 0
            || bakingOptions.glyphPaddingOuter.bottom < 0
            || bakingOptions.glyphPaddingOuter.left < 0
            || bakingOptions.glyphPaddingOuter.right < 0)
        throw FontError("Glyph outer padding should be >= 0");

    if (bakingOptions.glyphSpacing.x < 0
            || bakingOptions.glyphSpacing.y < 0)
        throw FontError("Glyph spacing should be >= 0");
}


void Font::sortGlyphs(GlyphsOrder newOrder)
{
    if (newOrder == glyphsOrder || newOrder == GlyphsOrder::unsorted)
        return;

    glyphsOrder = newOrder;
    switch (glyphsOrder) {
        case GlyphsOrder::unsorted:
            break;
        case GlyphsOrder::sizeDescending:
            struct CmpGlyphsBySizeDescending {
                bool operator()(const Glyph& a, const Glyph& b) const
                {
                    if (a.size.h != b.size.h)
                        return a.size.h > b.size.h;
                    else
                        return a.size.w > b.size.w;
                }
            };
            std::sort(
                glyphs.begin(), glyphs.end(), CmpGlyphsBySizeDescending());
            break;
        case GlyphsOrder::cp:
            struct CmpGlyphsByCp {
                bool operator()(const Glyph& a, const Glyph& b) const
                {
                    return a.cp < b.cp;
                }
            };
            std::sort(glyphs.begin(), glyphs.end(), CmpGlyphsByCp());
            break;
        case GlyphsOrder::glyphIdx:
            struct CmpGlyphsByGlyphIdx {
                bool operator()(const Glyph& a, const Glyph& b) const
                {
                    return a.glyphIdx < b.glyphIdx;
                }
            };
            std::sort(glyphs.begin(), glyphs.end(), CmpGlyphsByGlyphIdx());
            break;
    };
}


void Font::uploadGlyphs(const cp_range::CpRangeList& cpRangeList)
{
    // Font::getFontMetrics() returns metrics adjusted according to
    // the inner padding. We need original metrics, so get them
    // directly from the renderer.
    const auto ascender = renderer->getFontMetrics().ascender;

    for (const auto& cpRange : cpRangeList) {
        for (auto cp = cpRange.cpFirst; cp <= cpRange.cpLast; ++cp) {
            const auto glyphIdx = renderer->getGlyphIndex(cp);
            if (glyphIdx == 0 && cp != 0)
                continue;

            const auto glyphMetrics = renderer->getGlyphMetrics(glyphIdx);

            Glyph glyph;
            glyph.cp = cp;
            glyph.glyphIdx = glyphIdx;
            glyph.size = glyphMetrics.size;
            glyph.drawOffset.x = glyphMetrics.offset.x;
            glyph.drawOffset.y = ascender - glyphMetrics.offset.y;
            glyph.advance = glyphMetrics.advance;

            // Inner padding
            const auto xPaddingInner = (
                bakingOptions.glyphPaddingInner.left
                + bakingOptions.glyphPaddingInner.right);
            const auto yPaddingInner = (
                bakingOptions.glyphPaddingInner.top
                + bakingOptions.glyphPaddingInner.bottom);

            glyph.size.w += xPaddingInner;
            glyph.size.h += yPaddingInner;
            glyph.drawOffset.y -= bakingOptions.glyphPaddingInner.top;
            glyph.advance += xPaddingInner;

            // Outer padding
            glyph.drawOffset.x -= bakingOptions.glyphPaddingOuter.left;
            glyph.drawOffset.y -= bakingOptions.glyphPaddingOuter.top;
            glyph.size.w += (
                bakingOptions.glyphPaddingOuter.left
                + bakingOptions.glyphPaddingOuter.right);
            glyph.size.h += (
                bakingOptions.glyphPaddingOuter.top
                + bakingOptions.glyphPaddingOuter.bottom);

            glyphs.push_back(glyph);
        }
    }
}


void Font::packGlyphs()
{
    sortGlyphs(GlyphsOrder::sizeDescending);

    using Packer = dp::rect_pack::RectPacker<>;
    Packer packer(
        bakingOptions.imageMaxSize, bakingOptions.imageMaxSize,
        Packer::Spacing(
            bakingOptions.glyphSpacing.x,
            bakingOptions.glyphSpacing.y),
        Packer::Padding(
            bakingOptions.imagePadding.top,
            bakingOptions.imagePadding.bottom,
            bakingOptions.imagePadding.left,
            bakingOptions.imagePadding.right));
    for (auto& glyph : glyphs) {
        const auto result = packer.insert(glyph.size.w, glyph.size.h);

        switch (result.status) {
            case dp::rect_pack::InsertStatus::ok:
                glyph.pageIdx = result.pageIndex;
                glyph.pagePos.x = result.pos.x;
                glyph.pagePos.y = result.pos.y;
                break;
            case dp::rect_pack::InsertStatus::zeroSize:
                // Whitespace
                glyph.pageIdx = 0;
                break;
            case dp::rect_pack::InsertStatus::negativeSize:
                // Not our fault
                glyph.size = Size();
                glyph.pageIdx = 0;
                break;
            case dp::rect_pack::InsertStatus::rectTooBig:
                throw FontError(str::format(
                    "Glyph %s is too big (%ix%i) for a %ix%i px page",
                    unicode::cpToStr(glyph.cp),
                    glyph.size.w, glyph.size.h,
                    bakingOptions.imageMaxSize, bakingOptions.imageMaxSize));
                break;
        }
    }

    pages.reserve(packer.getNumPages());
    for (std::size_t i = 0; i < packer.getNumPages(); ++i) {
        Page page;
        packer.getPageSize(i, page.size.w, page.size.h);
        pages.push_back(page);
    }
}


// https://docs.microsoft.com/en-us/typography/opentype/spec/head
void Font::readHead()
{
    const auto tableOffset = sfntOffsetTable.getTableOffset(
        sfntTag('h', 'e', 'a', 'd'));
    if (tableOffset == 0)
        // "head" is a required table:
        throw StreamError("Font has no \"head\" table");

    fontStream.seek(
        tableOffset +
        // majorVersion, minorVersion
        + 2 * sizeof(std::uint16_t)
        // fontRevision, checkSumAdjustment, magicNumber
        + 3 * sizeof(std::uint32_t)
        // Flags
        + sizeof(std::uint16_t),
        SeekOrigin::set);
    head.unitsPerEm = fontStream.readU16Be();
    if (head.unitsPerEm == 0)
        throw StreamError("unitsPerEm in \"head\" table is 0");

    fontStream.seek(
        // Created and modified date
        2 * sizeof(std::uint64_t)
        // xMin, yMin, xMax, yMax
        + 4 * sizeof(std::uint16_t),
        SeekOrigin::cur);
    head.macStyle = fontStream.readU16Be();
}


// https://docs.microsoft.com/en-us/typography/opentype/spec/os2
void Font::readOs2()
{
    const auto tableOffset = sfntOffsetTable.getTableOffset(
        sfntTag('O', 'S', '/', '2'));
    hasOs2 = tableOffset != 0;
    if (!hasOs2)
        // "OS/2" is optional for Mac fonts
        return;

    fontStream.seek(
        tableOffset +
        // version
        + sizeof(std::int16_t)
        // xAvgCharWidth
        + sizeof(std::int16_t)
        // usWeightClass
        + sizeof(std::uint16_t)
        // usWidthClass
        + sizeof(std::uint16_t)
        // fsType
        + sizeof(std::uint16_t)
        // ySubscriptXSize
        + sizeof(std::int16_t)
        // ySubscriptYSize
        + sizeof(std::int16_t)
        // ySubscriptXOffset
        + sizeof(std::int16_t)
        // ySubscriptYOffset
        + sizeof(std::int16_t)
        // ySuperscriptXSize
        + sizeof(std::int16_t)
        // ySuperscriptYSize
        + sizeof(std::int16_t)
        // ySuperscriptXOffset
        + sizeof(std::int16_t)
        // ySuperscriptYOffset
        + sizeof(std::int16_t)
        // yStrikeoutSize
        + sizeof(std::int16_t)
        // yStrikeoutPosition
        + sizeof(std::int16_t)
        // sFamilyClass
        + sizeof(std::int16_t)
        // panose[10]
        + sizeof(std::uint8_t[10])
        // ulUnicodeRange1
        + sizeof(std::uint32_t)
        // ulUnicodeRange2
        + sizeof(std::uint32_t)
        // ulUnicodeRange3
        + sizeof(std::uint32_t)
        // ulUnicodeRange4
        + sizeof(std::uint32_t)
        // achVendID
        + sizeof(std::uint8_t[4]),
        SeekOrigin::set);

    os2.fsSelection = fontStream.readU16Be();
}


// https://www.microsoft.com/typography/otspec/name.htm
void Font::readFontName()
{
    const std::uint16_t platformIdWin = 3;
    const std::uint16_t languageIdWinEnglishUs = 0x0409;
    const std::uint16_t encodingIdWinUcs2 = 1;

    const auto tableOffset = sfntOffsetTable.getTableOffset(
        sfntTag('n', 'a', 'm', 'e'));
    if (tableOffset == 0)
        // "name" is a required table.
        throw FontError("Font has no \"name\" table");

    fontStream.seek(tableOffset, SeekOrigin::set);

    const auto format = fontStream.readU16Be();
    if (format != 0 && format != 1)
        throw FontError("Invalid \"name\" table format");

    auto count = fontStream.readU16Be();
    if (count == 0)
        throw FontError("\"name\" table has no records");

    const auto stringsOffset = fontStream.readU16Be();
    const auto storageOffset = tableOffset + stringsOffset;

    while (count--) {
        const auto platformId = fontStream.readU16Be();
        const auto encodingId = fontStream.readU16Be();
        const auto languageId = fontStream.readU16Be();
        const auto nameId = fontStream.readU16Be();
        const auto strByteLen = fontStream.readU16Be();
        const auto strOffset = fontStream.readU16Be();

        // We rely on the fact that name records are sorted.
        if (platformId < platformIdWin
                || languageId < languageIdWinEnglishUs)
            continue;
        else if (platformId > platformIdWin
                || encodingId > encodingIdWinUcs2
                || languageId > languageIdWinEnglishUs)
            break;

        if (nameId > 17)
            break;

        std::string* dst;
        switch (nameId) {
            case 1:
                dst = &fontName.groupFamily;
                break;
            case 2:
                dst = &fontName.style;
                break;
            case 16:  // Typographic Family
                dst = &fontName.family;
                break;
            case 17:  // Typographic Subfamily
                dst = &fontName.style;
                break;
            default:
                continue;
                break;
        }

        const auto pos = fontStream.getPosition();
        fontStream.seek(storageOffset + strOffset, SeekOrigin::set);

        std::vector<char16_t> utf16Name;
        utf16Name.resize(strByteLen / 2);
        for (auto& ch : utf16Name)
            ch = fontStream.readU16Be();

        *dst = unicode::utf16ToUtf8(
            &utf16Name[0], &utf16Name[utf16Name.size()]);

        fontStream.seek(pos, SeekOrigin::set);
    }

    // Fallback from id 16 to id 1. No need to do the same for
    // fontName.style (from id 17 to id 2) as it's done naturally
    // by ids order.
    if (fontName.family.empty())
        fontName.family = fontName.groupFamily;
}


char32_t Font::glyphIdxToCp(GlyphIndex glyphIdx) const
{
    assert(glyphsOrder == GlyphsOrder::glyphIdx);

    struct CmpGlyphIdx {
        bool operator()(
            const Glyph& a, const GlyphIndex& glyphIdx) const
        {
            return a.glyphIdx < glyphIdx;
        }
    };

    const auto iter = std::lower_bound(
        glyphs.begin(), glyphs.end(), glyphIdx, CmpGlyphIdx());
    if (iter != glyphs.end() && iter->glyphIdx == glyphIdx)
        return iter->cp;

    return 0;
}


void Font::readKerningPairs()
{
    const KerningParams kerningParams {
        bakingOptions.fontPxSize,
        head.unitsPerEm
    };

    std::vector<RawKerningPair> rawKerningPairs;
    if (bakingOptions.kerningSource == KerningSource::gpos
            || bakingOptions.kerningSource == KerningSource::kernAndGpos)
        rawKerningPairs = readKerningPairsGpos(
            fontStream, sfntOffsetTable, kerningParams);

    // According to the OpenType manual, the "kern" table should be
    // applied when there is no GPOS table, or if the GPOS table doesn't
    // contain any "kern" features for the resolved language.
    // https://docs.microsoft.com/en-us/typography/opentype/spec/recom
    if (bakingOptions.kerningSource == KerningSource::kern
            || (bakingOptions.kerningSource == KerningSource::kernAndGpos
                && rawKerningPairs.empty()))
        rawKerningPairs = readKerningPairsKern(
            fontStream, sfntOffsetTable, kerningParams);
    if (rawKerningPairs.empty())
        return;

    // Prepare for binary search
    sortGlyphs(GlyphsOrder::glyphIdx);

    std::size_t i = 0;
    while (i < rawKerningPairs.size()) {
        const auto& rawKerningPair = rawKerningPairs[i];

        assert(rawKerningPair.amount != 0);

        const auto glyphIdx1 = rawKerningPair.glyphIdx1;
        const auto cp1 = glyphIdxToCp(glyphIdx1);

        // The raw pairs are grouped by the first glyph index (but
        // not necessarily fully sorted in case of GPOS as we read
        // pairs for multiple languages), so we can avoid calling
        // glyphIdxToCp() every time for the first code point in a
        // sequence.
        while (i < rawKerningPairs.size()) {
            const auto& rawKerningPair = rawKerningPairs[i];
            if (rawKerningPair.glyphIdx1 != glyphIdx1)
                break;

            ++i;

            assert(rawKerningPair.amount != 0);

            if (cp1 == 0)
                continue;

            const auto cp2 = glyphIdxToCp(rawKerningPair.glyphIdx2);
            if (cp2 == 0)
                continue;

            kerningPairs.push_back({cp1, cp2, rawKerningPair.amount});
        }
    }
}
