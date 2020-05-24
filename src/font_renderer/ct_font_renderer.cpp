
#if DPFB_USE_CORETEXT

#include "font_renderer/font_renderer.h"

#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>



class CtFontRenderer : public FontRenderer {
public:
    explicit CtFontRenderer(const FontRendererArgs& args);

    FontMetrics getFontMetrics() const override;

    GlyphIndex getGlyphIndex(char32_t cp) const override;
    GlyphMetrics getGlyphMetrics(GlyphIndex glyphIdx) const override;
    void renderGlyph(GlyphIndex glyphIdx, Image& image) const override;
    virtual ~CtFontRenderer() override;
private:
    CTFontRef font;
};


CtFontRenderer::CtFontRenderer(const FontRendererArgs& args)
{
    CTFontDescriptorRef descriptor = CTFontManagerCreateFontDescriptorFromData(CFDataCreate(nullptr, static_cast<const UInt8*>(args.data), args.dataSize));
    if (!descriptor)
        throw FontRendererError("CoreText can't create descriptor");
    font = CTFontCreateWithFontDescriptor(descriptor, static_cast<CGFloat>(args.pxSize), nullptr);
    CFRelease(descriptor);
    if (!font)
        throw FontRendererError("CoreText can't init font");
}


FontMetrics CtFontRenderer::getFontMetrics() const
{
    FontMetrics metrics;

    metrics.ascender = ceil(CTFontGetAscent(font));
    metrics.descender = ceil(CTFontGetDescent(font));
    metrics.lineHeight = ceil(CTFontGetLeading(font) + CTFontGetAscent(font) + CTFontGetDescent(font));

    return metrics;
}


GlyphIndex CtFontRenderer::getGlyphIndex(char32_t cp) const
{
    CGGlyph glyph;
    UniChar ch = static_cast<UniChar>(cp);
    if (CTFontGetGlyphsForCharacters(font, &ch, &glyph, 1))
        return static_cast<GlyphIndex>(glyph);
    return 0;
}

GlyphMetrics CtFontRenderer::getGlyphMetrics(GlyphIndex glyphIdx) const
{
    GlyphMetrics glyphMetrics;
    CGGlyph glyph = static_cast<CGGlyph>(glyphIdx);

    CGSize advance;
    CTFontGetAdvancesForGlyphs(font, kCTFontOrientationDefault, &glyph, &advance, 1);
    glyphMetrics.advance = round(advance.width);

    CGRect boundingRect = CTFontGetBoundingRectsForGlyphs(font, kCTFontOrientationDefault, &glyph, nullptr, 1);
    glyphMetrics.size.w = ceil(boundingRect.size.width) + ceil(boundingRect.origin.x);
    glyphMetrics.offset.x = 0;

    glyphMetrics.size.h = ceil(boundingRect.size.height);
    glyphMetrics.offset.y = ceil(boundingRect.size.height) + ceil(boundingRect.origin.y);

    return glyphMetrics;
}


void CtFontRenderer::renderGlyph(GlyphIndex glyphIdx, Image& image) const
{
    CGGlyph glyph = static_cast<CGGlyph>(glyphIdx);
    if (image.getWidth() == 0 || image.getHeight() == 0)
        return;

    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceGray();
    CGContextRef context = CGBitmapContextCreate(image.getData(), image.getWidth(), image.getHeight(),
                                                 8, image.getPitch(), colorSpace, kCGImageAlphaOnly);

    CGFontRef cgFont = CTFontCopyGraphicsFont(font, nullptr);
    CGContextSetFont(context, cgFont);
    CGContextSetFontSize(context, CTFontGetSize(font));
    CGContextSetFillColorWithColor(context, CGColorGetConstantColor(kCGColorWhite));
    CGRect boundingRect = CTFontGetBoundingRectsForGlyphs(font, kCTFontOrientationDefault, &glyph, nullptr, 1);
    CGPoint position = boundingRect.origin;
    position.x = 0;
    position.y = -position.y;
    CGContextShowGlyphsAtPositions(context, &glyph, &position, 1);
    CGContextFlush(context);
    CGContextRelease(context);
    CFRelease(cgFont);
    CGColorSpaceRelease(colorSpace);
}

CtFontRenderer::~CtFontRenderer()
{
    CFRelease(font);
}


class CtFontRendererCreator : public FontRendererCreator {
public:
    CtFontRendererCreator()
        : FontRendererCreator("ct")
    {
    }

    const char* getDescription() const override
    {
        return "CoreText";
    }

    FontRenderer* create(const FontRendererArgs& args) const override
    {
        return new CtFontRenderer(args);
    }
};


static CtFontRendererCreator creatorInstance;


#endif  // DPFB_USE_CORETEXT
