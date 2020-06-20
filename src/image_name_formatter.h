
#pragma once

#include <cstddef>
#include <string>


namespace dpfb {


class ImageNameFormatter {
public:
    ImageNameFormatter(
        const std::string& prefix,
        std::size_t numImages,
        const std::string& extension);

    std::string getImageName(std::size_t imageIdx) const;
private:
    std::string prefix;
    int numDigits;
    std::string extension;
};


}
