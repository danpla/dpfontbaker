
#if DPFB_USE_STBTT

#if defined(__GNUC__) || defined(__clang__)
    // Clang understands both "GCC" and "clang" names
    #pragma GCC diagnostic ignored "-Wunused-function"
    #define STBTT_STATIC
#endif

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "font_renderer/font_renderer.h"


class StbFontRenderer : public dpfb::FontRenderer {
public:
    explicit StbFontRenderer(const dpfb::FontRendererArgs& args);

    dpfb::FontMetrics getFontMetrics() const override;

    dpfb::GlyphIndex getGlyphIndex(char32_t cp) const override;
    dpfb::GlyphMetrics getGlyphMetrics(
        dpfb::GlyphIndex glyphIdx) const override;
    void renderGlyph(
        dpfb::GlyphIndex glyphIdx, dpfb::Image& image) const override;
private:
    stbtt_fontinfo font;
    float scale;
};


StbFontRenderer::StbFontRenderer(const dpfb::FontRendererArgs& args)
{
    if (!stbtt_InitFont(&font, args.data, 0))
        throw dpfb::FontRendererError("stbtt can't init font");

    scale = stbtt_ScaleForMappingEmToPixels(&font, args.pxSize);
}


dpfb::FontMetrics StbFontRenderer::getFontMetrics() const
{
    dpfb::FontMetrics metrics;

    int lineGap;
    // First try "hhea" table
    stbtt_GetFontVMetrics(
        &font,
        &metrics.ascender, &metrics.descender, &lineGap);

    if (metrics.ascender == 0 && metrics.descender == 0)
        // Fall back "OS/2" table
        stbtt_GetFontVMetricsOS2(
            &font, &metrics.ascender, &metrics.descender, &lineGap);

    metrics.lineHeight = metrics.ascender - metrics.descender + lineGap;

    metrics.ascender *= scale;
    metrics.descender *= scale;
    metrics.lineHeight *= scale;

    return metrics;

}


dpfb::GlyphIndex StbFontRenderer::getGlyphIndex(char32_t cp) const
{
    return stbtt_FindGlyphIndex(&font, cp);
}


dpfb::GlyphMetrics StbFontRenderer::getGlyphMetrics(
    dpfb::GlyphIndex glyphIdx) const
{
    dpfb::GlyphMetrics glyphMetrics;

    stbtt_GetGlyphHMetrics(
        &font, glyphIdx, &glyphMetrics.advance, nullptr);
    glyphMetrics.advance *= scale;

    int xMin, xMax, yMin, yMax;
    stbtt_GetGlyphBitmapBox(
        &font, glyphIdx, scale, scale, &xMin, &yMin, &xMax, &yMax);

    glyphMetrics.size.w = xMax - xMin;
    glyphMetrics.size.h = yMax - yMin;
    glyphMetrics.offset.x = xMin;

    // stbtt converts coordinates so y increases down.
    // Positive yMin in stbtt is negative yMax in FreeType.
    // We use FreeType's convention, so negate back.
    glyphMetrics.offset.y = -yMin;

    return glyphMetrics;
}


void StbFontRenderer::renderGlyph(
    dpfb::GlyphIndex glyphIdx, dpfb::Image& image) const
{
    stbtt_MakeGlyphBitmap(
        &font,
        image.getData(),
        image.getWidth(),
        image.getHeight(),
        image.getPitch(),
        scale, scale, glyphIdx);
}


class StbFontRendererCreator : public dpfb::FontRendererCreator {
public:
    StbFontRendererCreator()
        : FontRendererCreator("stb")
    {

    }

    const char* getDescription() const override
    {
        return (
            "stb_truetype.h "
            // STB_TRUETYPE_H_VERSION is our custom macro created
            // by parsing stb_truetype.h in CMakeLists.txt.
            #ifdef STB_TRUETYPE_H_VERSION
            STB_TRUETYPE_H_VERSION " "
            #endif
            "(https://github.com/nothings/stb)");
    }

    dpfb::FontRenderer* create(
        const dpfb::FontRendererArgs& args) const override
    {
        return new StbFontRenderer(args);
    }
};


static StbFontRendererCreator creatorInstance;


#endif  // DPFB_USE_STBTT
