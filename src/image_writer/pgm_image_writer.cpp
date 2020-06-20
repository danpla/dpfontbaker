
#include "image_writer/image_writer.h"
#include "str.h"


class PgmImageWriter : public dpfb::ImageWriter {
public:
    PgmImageWriter();

    const char* getDescription() const override;

    void write(
        dpfb::streams::Stream& stream,
        const dpfb::Image& image) const override;
};


PgmImageWriter::PgmImageWriter()
    : ImageWriter("pgm", ".pgm")
{

}


const char* PgmImageWriter::getDescription() const
{
    return "Netpbm Portable Gray Map";
}


void PgmImageWriter::write(
    dpfb::streams::Stream& stream, const dpfb::Image& image) const
{
    stream.writeStr(
        dpfb::str::format(
            "P5\n%i %i\n255\n",
            image.getWidth(), image.getHeight()));
    for (int y = 0; y < image.getHeight(); ++y)
        stream.writeBuffer(
            image.getData() + y * image.getPitch(), image.getWidth());
}


static PgmImageWriter instance;
