
#if DPFB_USE_FREETYPE

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include "font_renderer/font_renderer.h"
#include "str.h"
#include "unicode.h"


template<typename T>
inline T ftFoor(T i)
{
    return i & -64;
}


template<typename T>
inline T ftCeil(T i)
{
    return (i + 63) & -64;
}


static std::string ftErrorToStr(FT_Error err)
{
    switch (err) {
        case FT_Err_Ok:
            return "Ok";
            break;
        case FT_Err_Unknown_File_Format:
            return "Unknown font format";
            break;
        case FT_Err_Invalid_File_Format:
            return "Corrupt font";
            break;
        case FT_Err_Invalid_Version:
            return "Invalid FreeType version";
            break;
        case FT_Err_Invalid_Argument:
            return "Invalid argument";
            break;
        default:
            return dpfb::str::format(
                "FreeType error 0x%02x (see fterrdef.h)", err);
            break;
    }
}


static FT_Library library;
static std::size_t libRefCount;


static void refLib()
{
    ++libRefCount;
    if (libRefCount > 1)
        return;

    auto err = FT_Init_FreeType(&library);
    if (err != FT_Err_Ok)
        throw dpfb::FontRendererError(
            "Error initializing FreeType: " + ftErrorToStr(err));
}


static void unrefLib()
{
    assert(libRefCount > 0);
    --libRefCount;
    if (libRefCount == 0)
        FT_Done_FreeType(library);
}


class FtFontRenderer : public dpfb::FontRenderer {
public:
    explicit FtFontRenderer(const dpfb::FontRendererArgs& args);
    ~FtFontRenderer();

    dpfb::FontMetrics getFontMetrics() const override;

    dpfb::GlyphIndex getGlyphIndex(char32_t cp) const override;
    dpfb::GlyphMetrics getGlyphMetrics(
        dpfb::GlyphIndex glyphIdx) const override;
    void renderGlyph(
        dpfb::GlyphIndex glyphIdx, dpfb::Image& image) const override;
private:
    FT_Face face;
    FT_UInt loadFlags;
};


FtFontRenderer::FtFontRenderer(const dpfb::FontRendererArgs& args)
{
    refLib();

    auto err = FT_New_Memory_Face(
        library, args.data, args.dataSize, 0, &face);
    if (err != FT_Err_Ok) {
        unrefLib();
        throw dpfb::FontRendererError(
            dpfb::str::format(
                "Can't open font: %s", ftErrorToStr(err).c_str()));
    }

    if (!face->charmap) {
        FT_Done_Face(face);
        unrefLib();
        throw dpfb::FontRendererError(
            "Font doesn't contain Unicode charmap");
    }

    err = FT_Set_Char_Size(face, args.pxSize * 64, 0, 72, 0);
    if (err != FT_Err_Ok) {
        FT_Done_Face(face);
        unrefLib();
        throw dpfb::FontRendererError(ftErrorToStr(err));
    }

    loadFlags = FT_LOAD_DEFAULT;
    if (args.hinting == dpfb::Hinting::light)
        loadFlags |= FT_LOAD_TARGET_LIGHT;
}


FtFontRenderer::~FtFontRenderer()
{
    FT_Done_Face(face);
    unrefLib();
}


dpfb::FontMetrics FtFontRenderer::getFontMetrics() const
{
    dpfb::FontMetrics metrics;
    metrics.ascender = face->size->metrics.ascender >> 6;
    metrics.descender = face->size->metrics.descender >> 6;
    metrics.lineHeight = face->size->metrics.height >> 6;
    return metrics;
}


dpfb::GlyphIndex FtFontRenderer::getGlyphIndex(char32_t cp) const
{
    return FT_Get_Char_Index(face, cp);
}


static char32_t idxToCp(FT_Face face, dpfb::GlyphIndex idx)
{
    FT_ULong cp;
    FT_UInt gidx;

    cp = FT_Get_First_Char(face, &gidx);
    while (gidx != 0) {
        if (gidx == idx)
            return cp;

        cp = FT_Get_Next_Char(face, cp, &gidx);
    }

    return 0;
}


dpfb::GlyphMetrics FtFontRenderer::getGlyphMetrics(
    dpfb::GlyphIndex glyphIdx) const
{
    auto err = FT_Load_Glyph(face, glyphIdx, loadFlags);
    if (err != FT_Err_Ok)
        throw dpfb::FontRendererError(
            dpfb::str::format(
                "Can't load glyph for %s: %s",
                dpfb::unicode::cpToStr(idxToCp(face, glyphIdx)),
                ftErrorToStr(err).c_str()));

    dpfb::GlyphMetrics glyphMetrics;
    glyphMetrics.advance = face->glyph->advance.x >> 6;

    if (face->glyph->format == FT_GLYPH_FORMAT_BITMAP) {
        // Embedded bitmap
        glyphMetrics.size.w = static_cast<int>(face->glyph->bitmap.width);
        glyphMetrics.size.h = static_cast<int>(face->glyph->bitmap.rows);
        glyphMetrics.offset.x = face->glyph->bitmap_left;
        glyphMetrics.offset.y = face->glyph->bitmap_top;
    } else {
        FT_BBox bbox;
        FT_Outline_Get_CBox(&face->glyph->outline, &bbox);

        // See:
        //  FreeType Glyph Conventions
        //      VI. FreeType outlines
        //          3. Coordinates, scaling and grid-fitting
        // https://www.freetype.org/freetype2/docs/glyphs/glyphs-6.html

        bbox.xMin = ftFoor(bbox.xMin);
        bbox.xMax = ftCeil(bbox.xMax);
        bbox.yMin = ftFoor(bbox.yMin);
        bbox.yMax = ftCeil(bbox.yMax);

        glyphMetrics.size.w = (bbox.xMax - bbox.xMin) >> 6;
        glyphMetrics.size.h = (bbox.yMax - bbox.yMin) >> 6;
        glyphMetrics.offset.x = bbox.xMin >> 6;
        glyphMetrics.offset.y = bbox.yMax >> 6;
    }

    return glyphMetrics;
}


void FtFontRenderer::renderGlyph(
    dpfb::GlyphIndex glyphIdx, dpfb::Image& image) const
{
    auto err = FT_Load_Glyph(face, glyphIdx, loadFlags);
    if (err != FT_Err_Ok)
        throw dpfb::FontRendererError(
            dpfb::str::format(
                "Can't load glyph for %s: %s",
                dpfb::unicode::cpToStr(idxToCp(face, glyphIdx)),
                ftErrorToStr(err).c_str()));

    err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
    if (err != FT_Err_Ok)
        throw dpfb::FontRendererError(
            dpfb::str::format(
                "Can't render glyph for %s: %s",
                dpfb::unicode::cpToStr(idxToCp(face, glyphIdx)),
                ftErrorToStr(err).c_str()));

    const auto* bitmap = &face->glyph->bitmap;

    const auto* src = bitmap->buffer;
    const auto srcPitch = bitmap->pitch;

    if (bitmap->pitch < 0)
        src += -bitmap->pitch * (bitmap->rows - 1);

    auto* dst = image.getData();

    // The size of the returned bitmap can be both smaller and larger
    // than grid-fitted bbox. The latter can only happen with fonts
    // containing buggy bytecode (like DejaVu Sans v2.37).
    const auto dstW = std::min(
        image.getWidth(), static_cast<int>(bitmap->width));
    const auto dstH = std::min(
        image.getHeight(), static_cast<int>(bitmap->rows));

    const auto dstPitch = image.getPitch();

    for (int y = 0; y < dstH; ++y) {
        std::memcpy(dst, src, dstW);
        dst += dstPitch;
        src += srcPitch;
    }
}


class FtFontRendererCreator : public dpfb::FontRendererCreator {
public:
    FtFontRendererCreator()
        : FontRendererCreator("ft")
    {

    }

    const char* getDescription() const override
    {
        static char buf[64];

        if (!buf[0]) {
            FT_Int vMajor;
            FT_Int vMinor;
            FT_Int vPatch;

            refLib();
            FT_Library_Version(library, &vMajor, &vMinor, &vPatch);
            unrefLib();

            std::snprintf(
                buf, sizeof(buf),
                "FreeType %i.%i.%i (https://www.freetype.org)",
                vMajor, vMinor, vPatch);
        }

        return buf;
    }

    dpfb::FontRenderer* create(
        const dpfb::FontRendererArgs& args) const override
    {
        return new FtFontRenderer(args);
    }
};


static FtFontRendererCreator creatorInstance;


#endif  // DPFB_USE_FREETYPE
