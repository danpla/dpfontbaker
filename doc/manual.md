
# About

dpFontBaker is a command-line bitmap font generator. It takes an
OTF/TTF font and generates a series of images containing tightly
packed glyph bitmaps, plus a description file to load and render them.

Features:

*   Rendering with [FreeType][] and [stb_truetype][]
*   Compatible with [BMFont][]
*   Simple C++ API to add custom font and image formats

[FreeType]: https://www.freetype.org/
[stb_truetype]: https://github.com/nothings/stb/blob/master/stb_truetype.h
[BMFont]: http://www.angelcode.com/products/bmfont/


# Downloads


# Usage

dpFontBaker is a command-line program. Run it with `-help` argument
to see all available options.


## Input font

dpFontBaker supports TrueType and OpenType fonts, including font
collections (.ttc, .otc). To select the font in a font collection,
use `-font-index` option.


## Font rendering


### Font size and DPI

`-font-size` specifies font size in points, and `-font-dpi` specifies
the rendering scale as dots per inch. The point is 1/72 of an inch,
so the default 72 DPI gives you font size in pixels (1:1 scale). You
probably don't want to touch `-font-dpi`: as with FreeType, your
application should query the system DPI at runtime, and then select
the proper pixel size using the formula `px = pt * dpi / 72`.


### Hinting

You can control hinting modes with `-hinting` option. Currently, it
only affects the FreeType renderer.

`-hinting light` enables [light hinting][ft-load-target-light]
mode for FreeType renderer, which usually looks better than normal
hinting, especially for small font sizes.

Be aware that hinting algorithms and settings vary between FreeType
versions, so rendered glyphs may look slightly different at small
sizes. This is especially important for Unix-like machines, where
dpFontBaker uses the system-wide FreeType library. If that's an
issue, you may compile the desired FreeType version and then use
`LD_LIBRARY_PATH` environment variable:

    export LD_LIBRARY_PATH=/path/to/your/freetype
    ./dpfb ...

For more info about hinting differences between FreeType versions,
see:

* [The new v40 TrueType interpreter mode][subpixel-hinting]
* [On Slight Hinting, Proper Text Rendering, Stem Darkening and LCD Filters][text-rendering-general]

[ft-load-target-light]: https://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#FT_LOAD_TARGET_LIGHT
[subpixel-hinting]: https://www.freetype.org/freetype2/docs/subpixel-hinting.html
[text-rendering-general]: https://www.freetype.org/freetype2/docs/text-rendering-general.html


## Code points

`-code-points` is a comma-separated list of Unicode code points and
code point ranges. A code point can be represented either as U+
followed by a hexadecimal sequence, or as a decimal number. A range
consists of two code points separated by a hyphen.

You can find the list of Unicode blocks (named code point ranges) in
[the Unicode Character Database][ucd] (file
[ucd/Blocks.txt][ucd-blocks]) or in
[the Wikipedia article][wikipedia-unicode-block]. You may also be
interested in [Unicode FAQ - Blocks and Ranges][faq-blocks-ranges].

Null, space, and replacement character (U+FFFD) are included even if
not mentioned in the `-code-points` list. The [Null glyph][notdef]
(also known as .notdef glyph) is used as a replacement for glyphs not
found in the font, and usually looks like an empty rectangle.

The following example shows the list containing Basic Latin and
Cyrillic blocks, White Smiling Face, and Object Replacement
Character:

    -code-points "U+0000-U+007F, U+0080-U+00FF, U+263A, U+FFFC"

The same example, written in a way more loose manner, shows that you
can omit whitespace and leading zeros, mix hexadecimal and decimal
forms, and use both upper and lower cases for hexadecimal sequences
(note that "U+" must be in upper case):

    -code-points "0-U+7F, U+000080 - 255,9786,U+fffc"

[wikipedia-unicode-block]: https://en.wikipedia.org/wiki/Unicode_block
[ucd]: http://www.unicode.org/ucd/
[ucd-blocks]: https://www.unicode.org/Public/UCD/latest/ucd/Blocks.txt
[faq-blocks-ranges]: https://www.unicode.org/faq/blocks_ranges.html
[notdef]: https://docs.microsoft.com/en-us/typography/opentype/spec/recom#glyph-0-the-notdef-glyph


## Kerning

dpFontBaker can extract kerning pairs from both "kern" and "GPOS"
tables. If you need compatibility with FreeType's `FT_Get_Kerning()`,
which only reads the "kern" table, use `-kerning kern`.

Please be aware that you will need a text shaping engine like
[HarfBuzz][] to use all text layout features provided by OpenType,
which include much more than just horizontal kerning.

[HarfBuzz]: https://www.freedesktop.org/wiki/Software/HarfBuzz/


## Glyph padding

Glyph padding enlarges bitmaps of glyphs so you can add some
post-effects. The are two types of glyph padding: inner
(`-glyph-padding-inner`) and outer (`-glyph-padding-outer`).

The inner padding is treated as a part of a glyph. You can use it to
add effects that should not overlap, like an outline. Since the inner
padding enlarges every glyph, it affects both glyph and font metrics.
In particular, the horizontal inner padding is added to every glyph's
advance, and vertical inner padding is added to the line height.

The outer padding is drawn "outside" a glyph, and therefore does not
affect glyph metrics. It's useful for effects that can overlap, like
a glow or a drop shadow.


## Glyph spacing and image padding

Glyph spacing (`-glyph-spacing`) and image padding (`-image-padding`)
prevents color bleeding when glyphs are drawn with
[interpolation and/or mipmapping][tex-filtering]. The default 1 px
spacing and padding is fine for bilinear filtering, but for mipmapping
you'll need to add more, depending on the texture size (you can use
`log2(max(width, height))`).

[tex-filtering]: https://en.wikipedia.org/wiki/Texture_filtering


## Export formats

dpFontBaker writes the baked font as a font description file and a set
of images containing glyph bitmaps.

### Font export format

dpFontBaker can write fonts in generic JSON and BMFont formats.

#### JSON

JSON is the main format of dpFontBaker. It contains all font
properties available in the internal C++ API, and most names in the
JSON file match API names. A JSON font is intended to be parsed with
a scripting language (like Python) to convert it to the
applications-specific font format and/or bitmap post-processing
(outlines, gradients, drop shadows, etc.).

The excellent FreeType's documentation contains detailed description
of various typographic concepts, so I will not repeat it here. In
particular, you will be interested in [Glyph Conventions][ft-glyphs].
The only thing that differs from FreeType is bitmap's y offset.
dpFontBaker use the same origin as the most graphics APIs, where
coordinates define the top left corner of an image, so `drawOffset.y`
from JSON font should be added to y drawing position. In FreeType,
however, the origin is placed on the baseline, and you need to
subtract `FT_GlyphSlotRec.bitmap_top` from the origin to get the
y coordinate for drawing. If you need the FreeType's offset,
subtract `drawOffset.y` from `metrics.ascender`.

The `name` object contains font family and style names. For example,
for DejaVu Serif Condensed Bold Italic they are "DejaVu Serif" and
"Condensed Bold Italic" respectively. `name.groupFamily` is used by
Windows' programs that can't work with font families containing more
than 4 styles (regular, italic, bold, and bold italic). BMfont is one
of them, so the group family was added primarily for compatibility
with BMFont font format. The `styleFlags` tells the style of the
group family. For the previous font example, the group family name is
"DejaVu Serif Condensed" and the style flags are `true`.

`pages[].size` is always the minimal page size and therefore can be
smaller than the actual image size if `-image-size-mode` is `minPot`
or `max`.

[ft-glyphs]: https://www.freetype.org/freetype2/docs/glyphs/index.html
[name-ids]: https://docs.microsoft.com/en-us/typography/opentype/spec/name#name-ids


#### BMFont

dpFontBaker has a built-in plugin for exporting fonts in [BMFont][]
(textual) format.

Be aware that BMFont always writes images of the same size, while
dpFontBaker creates variable sized images by default. You can use
`-image-size-mode max` force the BMFont's behavior. `scaleW` and
`scaleH` fields are always set to `-image-max-size` regardless of
`-image-size-mode`.


### Image format

Out of the box, dpFontBaker supports PGM, PNG and TGA image formats.
Images are in the grayscale 1 byte per pixel format; white glyphs on
the black background.

dpFontBaker creates RLE-compressed TGAs with top-left image origin.
They conform to the [TGA 2.0 specification][tga-spec] and are
identical to the files produced by GIMP 2.8. Since people usually
write their own TGA readers, here are some notes. The image type
(field 3) is 11 (grayscale RLE). The Y‑origin (field 5.2) is set to
image height. Pixel depth (field 5.5) is 8. The bits 5 and 4 of the
image descriptor (field 5.6) are set to 1 and 0 respectively (top
left origin).

[tga-spec]: http://www.dca.fee.unicamp.br/~martino/disciplinas/ea978/tgaffs.pdf


# Extra tools

dpFontBaker is shipped with several utilities that provide useful
features not included in the main program. They all need Python 3.4
or newer, and most of them also need [Pillow][] for image processing.
If you have never used Python programs before, please read
[Python Setup and Usage][] and [Installing Python Modules][].

Every script has a comprehensive `--help`.

[Pillow]: https://python-pillow.org/
[Python Setup and Usage]: https://docs.python.org/3/using/index.html
[Installing Python Modules]: https://docs.python.org/3/installing/index.html


## cp-list.py

`cp-list.py` extracts Unicode code points from text files for
dpFontBaker's `-code-points` option.


## draw-text.py

`draw-text.py` draws text with a JSON font to a PNG image.


## gray-add-alpha.py

`gray-add-alpha.py` adds alpha to grayscale images, threating black as
transparent and white as opaque. This is the same as applying the
input image as a mask to a white image of the same size.

The script will silently ignore all images that are not grayscale.


## split.py

`split.py` splits glyph pages of a JSON font, putting every glyph on a
separate PNG image. You can then assemble pages back with `join.py`.


## join.py

`join.py` join glyph images separated by `split.py` back to pages.

`join.py` creates pages in RGBA mode, assuming that alpha channels
have been added to glyph images during some post-processing (adding
outlines, gradients, etc.). To be more specific, every glyph image is
pasted at fully transparent white RGBA page.
