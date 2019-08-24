
#include "image.h"

#include <cstddef>
#include <stdexcept>


Image::Image(std::uint8_t* data, int w, int h, int pitch)
    : w {w}
    , h {h}
    , pitch {pitch}
    , data {data}
{
    if (w < 0)
        throw std::runtime_error("Width is < 0");
    if (h < 0)
        throw std::runtime_error("Height is < 0");
    if (pitch < w)
        throw std::runtime_error("Pitch < width");
    if (!data)
        throw std::runtime_error("Null data");
}


Image::Image(int w, int h)
    : w {w}
    , h {h}
    , pitch {w}
{
    if (w < 0)
        throw std::runtime_error("Width is < 0");
    if (h < 0)
        throw std::runtime_error("Height is < 0");

    ownData.resize(static_cast<std::size_t>(w) * h);
    data = ownData.data();

}

int Image::getWidth() const
{
    return w;
}


int Image::getHeight() const
{
    return h;
}


int Image::getPitch() const
{
    return pitch;
}


std::uint8_t* Image::getData()
{
    return data;
}


const std::uint8_t* Image::getData() const
{
    return data;
}
