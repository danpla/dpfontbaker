
#pragma once

#include <cstdint>
#include <vector>


namespace dpfb {


class Image {
public:
    Image(std::uint8_t* data, int w, int h, int pitch);
    Image(int w, int h);

    int getWidth() const;
    int getHeight() const;
    int getPitch() const;

    std::uint8_t* getData();
    const std::uint8_t* getData() const;
private:
    int w;
    int h;
    int pitch;
    std::uint8_t* data;
    std::vector<std::uint8_t> ownData;
};


}
