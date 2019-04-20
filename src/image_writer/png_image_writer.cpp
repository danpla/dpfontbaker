
#include <cstdio>
#include <memory>
#include <string>

#include <png.h>

#include "image_writer/image_writer.h"


class PngImageWriter : public ImageWriter {
public:
    PngImageWriter();

    const char* getDescription() const override;

    void write(
        streams::Stream& stream,
        const Image& image) const override;
};


PngImageWriter::PngImageWriter()
    : ImageWriter("png", ".png")
{

}


const char* PngImageWriter::getDescription() const
{
    return "Portable Network Graphics";
}


static void errorFn(png_structp png_ptr, png_const_charp error_msg)
{
    auto* errorStr = static_cast<std::string*>(png_get_error_ptr(png_ptr));
    *errorStr = std::string("libpng error: ") + error_msg;
}


static void warningFn(png_structp png_ptr, png_const_charp warning_msg)
{
    (void)png_ptr;
    std::printf("libpng warning: %s\n", warning_msg);
}


static void writeFn(png_structp png_ptr, png_bytep data, png_size_t size)
{
    auto* stream = static_cast<streams::Stream*>(png_get_io_ptr(png_ptr));
    stream->write(data, size);
}


static void flushFn(png_structp png_ptr)
{
    (void)png_ptr;
}


void PngImageWriter::write(
    streams::Stream& stream, const Image& image) const
{
    png_structp png_ptr;
    png_infop info_ptr;

    // Values of local variables changed between setjmp() and longjmp()
    // are undefined unless they are declared as volatile.
    //
    // Instead of declaring variables as volatile, it's easier to put
    // them in a struct and allocate it on heap beforehand.
    struct Data {
        std::string errorStr;
    };
    std::unique_ptr<Data> data(new Data());

    png_ptr = png_create_write_struct(
        PNG_LIBPNG_VER_STRING, &data->errorStr, errorFn, warningFn);
    if (!png_ptr)
        throw ImageWriterError("libpng can't create write struct");

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, nullptr);
        throw ImageWriterError("libpng can't create info struct");
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        throw ImageWriterError(data->errorStr);
    }

    png_set_write_fn(png_ptr, &stream, writeFn, flushFn);

    png_set_IHDR(
        png_ptr, info_ptr,
        image.getWidth(), image.getHeight(),
        8,
        PNG_COLOR_TYPE_GRAY,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png_ptr, info_ptr);

    for (int y = 0; y < image.getHeight(); ++y) {
        const auto* row = image.getData() + y * image.getPitch();
        png_write_row(png_ptr, const_cast<std::uint8_t*>(row));
    }

    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
}


static PngImageWriter instance;
