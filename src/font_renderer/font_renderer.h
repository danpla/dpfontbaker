
#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include "image.h"
#include "geometry.h"


namespace dpfb {


class FontRendererError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};


enum class Hinting {
    normal,
    light
};


struct FontRendererArgs {
    const std::uint8_t* data;
    std::size_t dataSize;
    int pxSize;
    Hinting hinting;
};


using GlyphIndex = std::uint_least32_t;


struct GlyphMetrics {
    /**
     * Size of the glyph's bitmap.
     */
    Size size;

    /**
     * Offset from the origin.
     *
     * This is the offset of the bottom left corner of the bitmap
     * from the origin on the baseline. Like in FreeType, the y
     * coordinate increases up.
     */
    Point offset;

    /**
     * X advance.
     */
    int advance;
};


struct FontMetrics {
    int ascender;
    int descender;
    int lineHeight;
};


class FontRenderer {
public:
    static bool exists(const char* name);
    static FontRenderer* create(
        const char* name, const FontRendererArgs& args);

    FontRenderer() = default;
    virtual ~FontRenderer() {};

    FontRenderer(const FontRenderer& other) = delete;
    FontRenderer& operator=(const FontRenderer& other) = delete;

    FontRenderer(FontRenderer&& other) = delete;
    FontRenderer& operator=(FontRenderer&& other) = delete;

    virtual FontMetrics getFontMetrics() const = 0;

    virtual GlyphIndex getGlyphIndex(char32_t cp) const = 0;
    virtual GlyphMetrics getGlyphMetrics(GlyphIndex glyphIdx) const = 0;
    virtual void renderGlyph(GlyphIndex glyphIdx, Image& image) const = 0;
};


class FontRendererCreator {
public:
    static const FontRendererCreator* find(const char* name);

    static const FontRendererCreator* getFirst();
    const FontRendererCreator* getNext() const;

    explicit FontRendererCreator(const char* name);
    virtual ~FontRendererCreator() {};

    const char* getName() const;
    virtual const char* getDescription() const = 0;

    virtual FontRenderer* create(const FontRendererArgs& args) const = 0;
private:
    static FontRendererCreator* list;

    const char* name;
    FontRendererCreator* next;
};


}
