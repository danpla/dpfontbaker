
#include "image_writer/image_writer.h"
#include "str.h"


class PgmImageWriter : public ImageWriter {
public:
    PgmImageWriter();

    const char* getDescription() const override;

    void write(
        streams::Stream& stream,
        const Image& image) const override;
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
    streams::Stream& stream, const Image& image) const
{
    stream.writeStr(
        str::format(
            "P5\n%i %i\n255\n",
            image.getWidth(), image.getHeight()));
    for (int y = 0; y < image.getHeight(); ++y)
        stream.writeBuffer(
            image.getData() + y * image.getPitch(), image.getWidth());
}


static PgmImageWriter instance;
