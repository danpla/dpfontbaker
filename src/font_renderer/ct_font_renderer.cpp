
#if DPFB_USE_CORE_TEXT

#include <cmath>

#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>

#include "font_renderer/font_renderer.h"
#include "str.h"
#include "unicode.h"


class CoreTextFontRenderer : public FontRenderer {
public:
    explicit CoreTextFontRenderer(const FontRendererArgs& args);
    ~CoreTextFontRenderer();

    FontMetrics getFontMetrics() const override;

    GlyphIndex getGlyphIndex(char32_t cp) const override;
    GlyphMetrics getGlyphMetrics(GlyphIndex glyphIdx) const override;
    void renderGlyph(GlyphIndex glyphIdx, Image& image) const override;
private:
    CTFontRef font;
};


CoreTextFontRenderer::CoreTextFontRenderer(const FontRendererArgs& args)
{
    const auto data = CFDataCreate(nullptr, args.data, args.dataSize);
    if (!data)
        throw FontRendererError("CFDataCreate for font data failed");

    const auto descriptor = CTFontManagerCreateFontDescriptorFromData(data);
    CFRelease(data);

    if (!descriptor)
        throw FontRendererError(
            "CTFontManagerCreateFontDescriptorFromData failed");

    font = CTFontCreateWithFontDescriptor(descriptor, args.pxSize, nullptr);
    CFRelease(descriptor);

    if (!font)
        throw FontRendererError("CTFontCreateWithFontDescriptor failed");
}


CoreTextFontRenderer::~CoreTextFontRenderer()
{
    CFRelease(font);
}


FontMetrics CoreTextFontRenderer::getFontMetrics() const
{
    FontMetrics metrics;

    metrics.ascender = CTFontGetAscent(font);
    metrics.descender = -CTFontGetDescent(font);
    metrics.lineHeight = (
        metrics.ascender - metrics.descender + CTFontGetDescent(font));

    return metrics;
}


GlyphIndex CoreTextFontRenderer::getGlyphIndex(char32_t cp) const
{
    char16_t utf16[2];
    const auto numUtf16Chars = unicode::encodeUtf16(cp, utf16);

    UniChar uniChars[2] = {
        static_cast<UniChar>(utf16[0]), static_cast<UniChar>(utf16[1])
    };

    CGGlyph cgGlyphs[2];
    CTFontGetGlyphsForCharacters(font, uniChars, cgGlyphs, numUtf16Chars);

    return cgGlyphs[0];
}


const auto extraPaddingForAntialiasing = 0;


GlyphMetrics CoreTextFontRenderer::getGlyphMetrics(GlyphIndex glyphIdx) const
{
    GlyphMetrics glyphMetrics;
    const CGGlyph glyph = glyphIdx;

    CGSize advance;
    CTFontGetAdvancesForGlyphs(
        font, kCTFontOrientationDefault, &glyph, &advance, 1);
    glyphMetrics.advance = advance.width;

    CGRect boundingRect;
    CTFontGetBoundingRectsForGlyphs(
        font, kCTFontOrientationDefault, &glyph, &boundingRect, 1);

    const int xMin = std::floor(boundingRect.origin.x);
    const int xMax = std::ceil(
        boundingRect.origin.x + boundingRect.size.width);
    const int yMin = std::floor(boundingRect.origin.y);
    const int yMax = std::ceil(
        boundingRect.origin.y + boundingRect.size.height);

    glyphMetrics.size.w = xMax - xMin + extraPaddingForAntialiasing * 2;
    glyphMetrics.size.h = yMax - yMin + extraPaddingForAntialiasing * 2;
    glyphMetrics.offset.x = xMin - extraPaddingForAntialiasing;
    glyphMetrics.offset.y = yMax + extraPaddingForAntialiasing;

    return glyphMetrics;
}


void CoreTextFontRenderer::renderGlyph(GlyphIndex glyphIdx, Image& image) const
{
    // CGBitmapContextCreate() doesn't expect zero size.
    if (image.getWidth() == 0 || image.getHeight() == 0)
        return;

    const auto colorSpace = CGColorSpaceCreateDeviceGray();
    if (!colorSpace)
        throw FontRendererError("CGColorSpaceCreateDeviceGray failed");

    const auto context = CGBitmapContextCreate(
        image.getData(), image.getWidth(), image.getHeight(),
        8, image.getPitch(), colorSpace, kCGImageAlphaOnly);
    CFRelease(colorSpace);

    if (!context)
        throw FontRendererError("CGBitmapContextCreate failed");

    CGContextSetFillColorWithColor(
        context, CGColorGetConstantColor(kCGColorWhite));

    const CGGlyph glyph = glyphIdx;

    CGRect boundingRect;
    CTFontGetBoundingRectsForGlyphs(
        font, kCTFontOrientationDefault, &glyph, &boundingRect, 1);
    CGPoint position = boundingRect.origin;

    position.x = -std::floor(position.x) + extraPaddingForAntialiasing;
    position.y = -std::floor(position.y) + extraPaddingForAntialiasing;

    CTFontDrawGlyphs(font, &glyph, &position, 1, context);
    CGContextFlush(context);
    CFRelease(context);
}


class CoreTextFontRendererCreator : public FontRendererCreator {
public:
    CoreTextFontRendererCreator()
        : FontRendererCreator("core-text")
    {

    }

    const char* getDescription() const override
    {
        return "Core Text (macOS)";
    }

    FontRenderer* create(const FontRendererArgs& args) const override
    {
        return new CoreTextFontRenderer(args);
    }
};


static CoreTextFontRendererCreator creatorInstance;


#endif  // DPFB_USE_CORE_TEXT
