
#include <cassert>
#include <cstring>

#include "byteorder.h"
#include "image_writer/image_writer.h"


class TgaImageWriter : public ImageWriter {
public:
    TgaImageWriter();

    const char* getDescription() const override;

    void write(
        streams::Stream& stream,
        const Image& image) const override;
};


TgaImageWriter::TgaImageWriter()
    : ImageWriter("tga", ".tga")
{

}


const char* TgaImageWriter::getDescription() const
{
    return "Truevision TGA";
}


enum TgaType {
    tgaGrayscaleRle = 11,
};


enum TgaOrigin {
    tgaOriginTopLeft = 2,
};


#pragma pack(push, 1)
struct TgaHeader {
    std::uint8_t idLength;
    std::uint8_t colorMapType;
    std::uint8_t type;
    std::uint16_t cMapStart;
    std::uint16_t cMapLength;
    std::uint8_t cMapDepth;
    std::uint16_t xOffset;
    std::uint16_t yOffset;
    std::uint16_t width;
    std::uint16_t height;
    std::uint8_t depth;
    std::uint8_t descriptor;
};
#pragma pack(pop)


const char* tgaFooter = (
    "\x00\x00\x00\x00\x00\x00\x00\x00TRUEVISION-XFILE.\x00");
const int tgaFooterSize = 26;


static inline bool comparePixels(const std::uint8_t* data, int bytesPerPixel)
{
    return std::memcmp(data, data + bytesPerPixel, bytesPerPixel) == 0;
}


static void writeRleRow(
    streams::Stream& stream,
    const std::uint8_t* row, int w, int bytesPerPixel)
{
    const auto* rowEnd = row + w * bytesPerPixel;
    while (row < rowEnd) {
        // Start with a raw packet for 1 px.
        std::uint8_t descriptor = 0;
        const auto* sequenceStart = row;
        int sequenceLen = bytesPerPixel;

        if (row + bytesPerPixel < rowEnd) {
            const auto isRaw = !comparePixels(row, bytesPerPixel);
            row += bytesPerPixel;

            // A packet can contain up tp 128 pixels;
            // 2 are already behind.
            const auto* maxLookup = std::min(
                rowEnd - bytesPerPixel, row + 126 * bytesPerPixel);

            if (isRaw) {
                while (row < maxLookup)
                    if (!comparePixels(row, bytesPerPixel))
                        row += bytesPerPixel;
                    else {
                        // Two identical pixels will go to RLE packet.
                        row -= bytesPerPixel;
                        break;
                    }

                sequenceLen += row - sequenceStart;
            } else {
                descriptor |= 0x80;

                while (row < maxLookup)
                    if (comparePixels(row, bytesPerPixel))
                        row += bytesPerPixel;
                    else
                        break;
            }
        }

        descriptor += (row - sequenceStart) / bytesPerPixel;
        stream.writeU8(descriptor);
        stream.writeBuffer(sequenceStart, sequenceLen);

        row += bytesPerPixel;
    }
}


void TgaImageWriter::write(
    streams::Stream& stream, const Image& image) const
{
    TgaHeader header {};
    header.type = tgaGrayscaleRle;
    header.yOffset = byteorder::toLe(
        static_cast<std::uint16_t>(image.getHeight()));
    header.width = byteorder::toLe(
        static_cast<std::uint16_t>(image.getWidth()));
    header.height = byteorder::toLe(
        static_cast<std::uint16_t>(image.getHeight()));
    header.depth = 8;
    header.descriptor = tgaOriginTopLeft << 4;

    stream.writeBuffer(&header, sizeof(header));

    for (int y = 0; y < image.getHeight(); ++y)
        writeRleRow(
            stream,
            image.getData() + y * image.getPitch(),
            image.getWidth(),
            1);

    stream.writeBuffer(tgaFooter, tgaFooterSize);
}


static TgaImageWriter instance;
