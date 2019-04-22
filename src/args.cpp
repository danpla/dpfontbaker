
#include "args.h"

#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "font_renderer/font_renderer.h"
#include "font_writer/font_writer.h"
#include "image_writer/image_writer.h"
#include "version.h"


namespace args {


const int numPositionalArgs = 1;


const char* fontPath = "";

const char* codePoints = "33-126";
int fontDpi = 72;
const char* fontExportFormat = "json";
const char* fontExportName = "";
int fontIndex = 0;
const char* fontRenderer = (
    #if DPFB_USE_FREETYPE
    "ft"
    #else
    // We will pick the first available
    ""
    #endif
);
int fontSize = 16;
int glyphPaddingInner[4];
int glyphPaddingOuter[4];
int glyphSpacing[2] = {1, 1};
const char* hinting = "normal";
const char* imageFormat = (
    #if DPFB_USE_LIBPNG
    "png"
    #else
    // We will pick the first available
    ""
    #endif
);
int imageMaxCount = 30;
int imageMaxSize = 1024;
int imagePadding[4] = {1, 1, 1, 1};
const char* imageSizeMode = "min";
const char* kerning = "both";
const char* outDir = ".";


const char* help = (
    "dpFontBaker %s"
    "\n"
    "Bitmap font generator\n"
    "\n"
    "Usage: %s [options...] font-path\n"
    "\n"
    "  font-path\n"
    "           Path to a font\n"
    "\n"
    "  -code-points POINTS\n"
    "           Code points to bake. Default is \"%s\".\n"
    "  -font-dpi DPI\n"
    "           Font dpi. Default is %i.\n"
    "  -font-export-format NAME\n"
    "           Font export format. Default is \"%s\".\n"
    "  -font-export-name NAME\n"
    "           Name of the exported font. Default is the font file\n"
    "           name.\n"
    "  -font-index\n"
    "           0-based index of font in a collection (TTC and OTC).\n"
    "           Default is 0.\n"
    "  -font-size SIZE\n"
    "           Font size. Default is %i.\n"
    "  -font-renderer NAME\n"
    "           Font renderer. Default is \"%s\".\n"
    "  -help\n"
    "           Print this help and exit.\n"
    "  -hinting MODE\n"
    "           Hinting mode. Default is \"%s\".\n"
    "  -image-format NAME\n"
    "           Image format. Default is \"%s\".\n"
    "  -glyph-padding-inner TOP[:BOTTOM:LEFT:RIGHT]\n"
    "           Glyph padding that will be drawn as part of the glyph\n"
    "           (like an outline). Default is 0.\n"
    "  -glyph-padding-outer TOP[:BOTTOM:LEFT:RIGHT]\n"
    "           Glyph padding that will be drawn outside the glyph\n"
    "           (like a drop shadow). Default is 0.\n"
    "  -glyph-spacing X[:Y]\n"
    "           Spacing between glyph images. Default is 1.\n"
    "  -image-max-count COUNT\n"
    "           Maximum number of images. Default is %i.\n"
    "  -image-max-size SIZE\n"
    "           Image size limit. Default is %i.\n"
    "  -image-padding TOP[:BOTTOM:LEFT:RIGHT]\n"
    "           Image padding. Default is 1.\n"
    "  -image-size-mode MODE\n"
    "           Image size mode. Default is \"%s\".\n"
    "  -kerning SOURCE\n"
    "           Source of kerning pairs. Default is \"both\".\n"
    "  -out-dir PATH\n"
    "           Output directory. Default is \".\".\n"
    "  -version\n"
    "           Print program version and exit.\n"
    "\n"
    "Hinting modes (-hinting):\n"
    "  normal  normal hinting\n"
    "  light   light hinting; may look better than normal\n"
    "\n"
    "Kerning pairs source (-kerning):\n"
    "  none  don't extract kerning pairs\n"
    "  kern  extract pairs from \"kern\" table\n"
    "  gpos  extract pairs from \"GPOS\" table\n"
    "  both  extract pairs from both \"kern\" and \"GPOS\" tables\n"
    "\n"
    "Image size modes (-image-size-mode):\n"
    "  min     use minimal image size\n"
    "  minPot  use minimal power of two image size <= -image-max-size\n"
    "  max     force all images to -image-max-size\n"
    "\n"
);


template<typename T>
void listPlugins()
{
    std::size_t maxNameLen = 0;
    for (const auto* p = T::getFirst(); p; p = p->getNext()) {
        const auto len = std::strlen(p->getName());
        if (len > maxNameLen)
            maxNameLen = len;
    }

    for (const auto* p = T::getFirst(); p; p = p->getNext())
        std::printf(
            "  %-*s  %s\n",
            static_cast<int>(maxNameLen),
            p->getName(),
            p->getDescription());
    std::printf("\n");
}


static void printHelp(const char* progName)
{
    std::printf(
        help,
        dpfbVersion,
        progName,
        codePoints,
        fontDpi, fontExportFormat, fontSize, fontRenderer,
        hinting,
        imageFormat,
        imageMaxCount, imageMaxSize, imageSizeMode);

    std::printf("Font export formats (-font-export-format):\n");
    listPlugins<FontWriter>();

    std::printf("Font renderers (-font-renderer):\n");
    listPlugins<FontRendererCreator>();

    std::printf("Image formats (-image-format):\n");
    listPlugins<ImageWriter>();
}


static const char* varNameToArgName(const char* name)
{
    static std::string nameStr;
    nameStr.clear();

    nameStr += '-';

    while (*name) {
        const auto c = *name;
        ++name;

        if (std::isupper(c)) {
            nameStr += '-';
            nameStr += std::tolower(c);
        } else
            nameStr += c;
    }

    return nameStr.c_str();
}


static void advanceToArg(char**& cursor, char** optArgsEnd)
{
    assert(cursor < optArgsEnd);

    ++cursor;

    if (cursor == optArgsEnd) {
        std::fprintf(stderr, "%s expects an argument\n", *(cursor - 1));
        std::exit(EXIT_FAILURE);
    }
}


static void getValue(char**& cursor, char** optArgsEnd, bool& var)
{
    (void)cursor;
    (void)optArgsEnd;

    assert(!var);
    var = true;
}


static void getValue(
    char**& cursor, char** optArgsEnd, const char*& var)
{
    advanceToArg(cursor, optArgsEnd);

    assert(var);
    var = *cursor;
}


static void getValue(char**& cursor, char** optArgsEnd, int& var)
{
    advanceToArg(cursor, optArgsEnd);

    char* end;
    var = std::strtol(*cursor, &end, 10);
    if (*cursor == end) {
        std::fprintf(stderr, "Invalid %s: %s\n", *(cursor - 1), *cursor);
        std::exit(EXIT_FAILURE);
    }
}


static bool parseIntArray(
    const char* str, int array[], std::size_t arraySize)
{
    for (std::size_t i = 0; i < arraySize; ++i) {
        char* end;
        array[i] = std::strtol(str, &end, 10);

        if (str == end)
            return false;
        else if (*end == ':') {
            if (i < arraySize - 1)
                str = end + 1;
            else
                return false;
        } else if (*end == 0) {
            if (i == 0) {
                // Single value
                for (auto j = i + 1; j < arraySize; ++j)
                    array[j] = array[i];
                break;
            } else if (i < arraySize - 1)
                return false;
        } else
            return false;
    }

    return true;
}


template<std::size_t N>
void getValue(char**& cursor, char** optArgsEnd, int (&var)[N])
{
    advanceToArg(cursor, optArgsEnd);

    if (!parseIntArray(*cursor, var, N)) {
        std::fprintf(stderr, "Invalid %s: %s\n", *(cursor - 1), *cursor);
        std::exit(EXIT_FAILURE);
    }
}


#define OPT(VAR_NAME) \
    if (std::strcmp(*cursor, varNameToArgName(#VAR_NAME)) == 0) { \
        getValue(cursor, optArgsEnd, VAR_NAME); \
        continue; \
    } \


template<typename T>
void pickDefaultPlugin(const char*& arg, const char* errorMsg)
{
    if (arg[0])
        return;

    const auto* p = T::getFirst();
    if (!p) {
        std::fputs(errorMsg, stderr);
        std::exit(EXIT_FAILURE);
    }
    arg = p->getName();
}


void parse(int argc, char* argv[])
{
    pickDefaultPlugin<FontRendererCreator>(
        fontRenderer,
        "All font renderers was disabled at compile time\n");

    pickDefaultPlugin<ImageWriter>(
        fontRenderer,
        "All image writers was disabled at compile time\n");

    for (int i = 1; i < argc; ++i)
        if (std::strcmp(argv[i], "-help") == 0) {
            printHelp(argv[0]);
            std::exit(EXIT_SUCCESS);
        } else if (std::strcmp(argv[i], "-version") == 0) {
            std::printf("%s\n", dpfbVersion);
            std::exit(EXIT_SUCCESS);
        }

    if (argc < 1 + numPositionalArgs) {
        std::fprintf(
            stderr,
            "Expected %i positional argument%s\n",
            numPositionalArgs,
            numPositionalArgs > 1 ? "s" : "");
        std::exit(EXIT_FAILURE);
    }

    fontPath = argv[argc - 1];

    auto** optArgsEnd = argv + (argc - numPositionalArgs);
    for (auto** cursor = argv + 1; cursor < optArgsEnd; ++cursor) {
        OPT(codePoints);
        OPT(fontDpi);
        OPT(fontExportFormat);
        OPT(fontExportName);
        OPT(fontIndex);
        OPT(fontRenderer);
        OPT(fontSize);
        OPT(glyphPaddingInner);
        OPT(glyphPaddingOuter);
        OPT(glyphSpacing);
        OPT(hinting);
        OPT(imageFormat);
        OPT(imageMaxCount);
        OPT(imageMaxSize);
        OPT(imagePadding);
        OPT(imageSizeMode);
        OPT(kerning);
        OPT(outDir);

        std::fprintf(stderr, "Unknown option %s\n", *cursor);
        std::exit(EXIT_FAILURE);
    }
}


}
