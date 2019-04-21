
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "args.h"
#include "cp_range.h"
#include "font.h"
#include "font_writer/font_writer.h"
#include "geometry.h"
#include "image.h"
#include "image_writer/image_writer.h"
#include "image_name_formatter.h"
#include "str.h"
#include "streams/file_stream.h"
#include "unicode.h"


const char* const dirSeparators = (
    #ifdef _WIN32
        "\\/"
    #else
        "/"
    #endif
);


static std::string getFontExportNameFromPath(const std::string& fontPath)
{
    std::string result;

    // Remove directory
    const auto sepPos = fontPath.find_last_of(dirSeparators);
    if (sepPos == fontPath.npos)
        result = fontPath;
    else
        result = fontPath.substr(sepPos + 1);

    // Remove extension
    const auto periodPos = result.rfind('.');
    if (periodPos != fontPath.npos)
        result.resize(periodPos);

    return result;
}


static void addTrailingPathSeparator(std::string& path)
{
    if (path.empty() || std::strchr(dirSeparators, path.back()))
        return;

    path += dirSeparators[0];
}


static cp_range::CpRangeList createCpRangeList()
{
    cp_range::CpRangeList result;
    try {
        result = cp_range::parse(args::codePoints);
    } catch (cp_range::CpRangeError& e) {
        throw std::runtime_error(str::format(
            "Invalid code points: %s", e.what()));
    }

    const char32_t guaranteedCodePoints[] = {
        0,  // .notdef ("missing" glyph)
        ' ',
        unicode::replacementCharacter,
    };
    for (auto cp : guaranteedCodePoints)
        result.push_back(cp_range::CpRange(cp));

    cp_range::compress(result);

    return result;
}


static int ptToPx(int pt, int dpi)
{
    return (pt * dpi + 36) / 72;
}


static FontBakingOptions createFontBakingOptions()
{
    Hinting hinting;
    if (std::strcmp(args::hinting, "normal") == 0)
        hinting = Hinting::normal;
    else if (std::strcmp(args::hinting, "light") == 0)
        hinting = Hinting::light;
    else
        throw std::runtime_error(str::format(
            "Invalid hinting \"%s\"", args::hinting));

    KerningSource kerningSource;
    if (std::strcmp(args::kerning, "none") == 0)
        kerningSource = KerningSource::none;
    else if (std::strcmp(args::kerning, "kern") == 0)
        kerningSource = KerningSource::kern;
    else if (std::strcmp(args::kerning, "gpos") == 0)
        kerningSource = KerningSource::gpos;
    else if (std::strcmp(args::kerning, "both") == 0)
        kerningSource = KerningSource::kernAndGpos;
    else
        throw std::runtime_error(str::format(
            "Invalid kerning \"%s\"", args::kerning));

    return {
        args::fontPath,
        args::fontRenderer,
        args::fontIndex,
        ptToPx(args::fontSize, args::fontDpi),
        hinting,
        args::imageMaxSize,
        Edge(
            args::imagePadding[0],
            args::imagePadding[1],
            args::imagePadding[2],
            args::imagePadding[3]),
        Edge(
            args::glyphPaddingInner[0],
            args::glyphPaddingInner[1],
            args::glyphPaddingInner[2],
            args::glyphPaddingInner[3]),
        Edge(
            args::glyphPaddingOuter[0],
            args::glyphPaddingOuter[1],
            args::glyphPaddingOuter[2],
            args::glyphPaddingOuter[3]),
        Point(args::glyphSpacing[0], args::glyphSpacing[1]),
        kerningSource
    };
}


enum class ImageSizeMode {
    min,
    minPot,
    max
};


struct ExportOptions {
    std::string exportName;
    std::string fontFormat;
    std::string imageFormat;
    int imageMaxCount;
    ImageSizeMode imageSizeMode;
    std::string outDir;
};


static ExportOptions createExportOpions()
{
    std::string exportName = args::fontExportName;
    if (exportName.empty())
        exportName = getFontExportNameFromPath(args::fontPath);

    if (args::imageMaxCount <= 0)
        throw std::runtime_error("Image max count should be > 0");

    ImageSizeMode imageSizeMode;
    if (std::strcmp(args::imageSizeMode, "min") == 0)
        imageSizeMode = ImageSizeMode::min;
    else if (std::strcmp(args::imageSizeMode, "minPot") == 0)
        imageSizeMode = ImageSizeMode::minPot;
    else if (std::strcmp(args::imageSizeMode, "max") == 0)
        imageSizeMode = ImageSizeMode::max;
    else
        throw std::runtime_error(str::format(
            "Invalid image size mode \"%s\"", args::imageSizeMode));

    std::string outDir = args::outDir;
    addTrailingPathSeparator(outDir);

    return {
        exportName,
        args::fontExportFormat,
        args::imageFormat,
        args::imageMaxCount,
        imageSizeMode,
        outDir
    };
}


static int pot(int n)
{
    int result = 1;
    while (result < n)
        result *= 2;
    return result;
}


static Size getMaxImageSize(
    const std::vector<Page>& pages,
    ImageSizeMode imageSizeMode,
    int imageMaxSize)
{
    if (imageSizeMode == ImageSizeMode::max)
        return {imageMaxSize, imageMaxSize};

    Size maxSize;
    for (const auto& page : pages) {
        if (maxSize.w < page.size.w)
            maxSize.w = page.size.w;
        if (maxSize.h < page.size.h)
            maxSize.h = page.size.h;
    }

    if (imageSizeMode == ImageSizeMode::minPot) {
        maxSize.w = std::min(pot(maxSize.w), imageMaxSize);
        maxSize.h = std::min(pot(maxSize.h), imageMaxSize);
    }

    return maxSize;
}


static Size getImageSize(
    const Size& pageSize,
    ImageSizeMode imageSizeMode,
    int imageMaxSize)
{
    if (imageSizeMode == ImageSizeMode::max)
        return {imageMaxSize, imageMaxSize};

    auto result = pageSize;
    if (imageSizeMode == ImageSizeMode::minPot) {
        result.w = std::min(pot(result.w), imageMaxSize);
        result.h = std::min(pot(result.h), imageMaxSize);
    }

    return result;
}


static void writeImages(
    const Font& font, const ImageNameFormatter& imageNameFormatter,
    const ImageWriter& imageWriter, const ExportOptions& exportOptions)
{
    const auto& pages = font.getPages();
    const auto imageMaxSize = font.getBakingOptions().imageMaxSize;

    const auto canvasSize = getMaxImageSize(
        pages, exportOptions.imageSizeMode, imageMaxSize);

    Image canvas(canvasSize.w, canvasSize.h);

    for (std::size_t pageIdx = 0; pageIdx < pages.size(); ++pageIdx) {
        std::memset(
            canvas.getData(),
            0,
            static_cast<std::size_t>(canvasSize.w) * canvasSize.h);

        const auto& page = pages[pageIdx];
        for (const auto glyphIdx : page.glyphIndices) {
            const auto& glyph = font.getGlyphs()[glyphIdx];

            Image glyphImage(
                canvas.getData()
                    + glyph.pagePos.y * canvas.getPitch() + glyph.pagePos.x,
                glyph.size.w,
                glyph.size.h,
                canvas.getPitch());
            try {
                font.renderGlyph(glyph.glyphIdx, glyphImage);
            } catch (FontRendererError& e) {
                throw std::runtime_error(str::format(
                    "%s font renderer can't render glyph for %s: %s",
                    font.getBakingOptions().fontRenderer.c_str(),
                    unicode::cpToStr(glyph.cp),
                    e.what()));
            }
        }

        const auto imagePath = (
            exportOptions.outDir + imageNameFormatter.getImageName(pageIdx));
        const auto imageSize = getImageSize(
            page.size, exportOptions.imageSizeMode, imageMaxSize);
        const Image image(
            canvas.getData(),
            imageSize.w,
            imageSize.h,
            canvas.getPitch());
        try {
            streams::FileStream f(imagePath, "wb");
            imageWriter.write(f, image);
        } catch (std::runtime_error& e) {
            // ImageWriterError and StreamError
            throw std::runtime_error(str::format(
                "%s image writer can't write \"%s\": %s",
                imageWriter.getName(),
                imagePath.c_str(),
                e.what()));
        }
    }
}


static void writeFont(
    const Font& font, const ImageNameFormatter& imageNameFormatter,
    const FontWriter& fontWriter, const ExportOptions& exportOptions)
{
    const auto fontPath = (
        exportOptions.outDir
        + exportOptions.exportName
        + fontWriter.getFileExtension());

    try {
        streams::FileStream f(fontPath, "wb");
        fontWriter.write(f, font, imageNameFormatter);
    } catch (std::runtime_error& e) {
        // FontWriterError and StreamError
        throw std::runtime_error(str::format(
            "%s font writer can't write \"%s\": %s",
            fontWriter.getName(), fontPath.c_str(), e.what()));
    }
}


static void bake()
{
    const auto cpRangeList = createCpRangeList();
    const auto bakingOptions = createFontBakingOptions();
    const auto exportOptions = createExportOpions();

    // Get writers early for validation
    const auto& imageWriter = ImageWriter::get(
        exportOptions.imageFormat.c_str());
    const auto& fontWriter = FontWriter::get(
        exportOptions.fontFormat.c_str());

    const Font font(bakingOptions, cpRangeList);

    const auto imageCount = font.getPages().size();
    if (imageCount > static_cast<std::size_t>(exportOptions.imageMaxCount))
        throw FontError(str::format(
            "The max image count (%zu) exceeds the user limit (%i). "
            "Please increase the maximum image count or image size limit.",
            imageCount, exportOptions.imageMaxCount));

    const ImageNameFormatter imageNameFormatter(
        exportOptions.exportName,
        imageCount,
        imageWriter.getFileExtension());

    writeFont(font, imageNameFormatter, fontWriter, exportOptions);
    writeImages(font, imageNameFormatter, imageWriter, exportOptions);
}


int main(int argc, char* argv[])
{
    args::parse(argc, argv);

    try {
        bake();
    } catch (std::runtime_error& e) {
        std::fprintf(
            stderr, "Can't bake %s: %s\n", args::fontPath, e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
