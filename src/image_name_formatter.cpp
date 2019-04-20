
#include "image_name_formatter.h"

#include <cassert>

#include "str.h"


template<typename T>
static int numDigits(T i)
{
    assert(i >= 0);

    int n = 1;
    while (i /= 10)
        ++n;
    return n;
}


ImageNameFormatter::ImageNameFormatter(
        const std::string& prefix,
        std::size_t numImages,
        const std::string& extension)
    : prefix {prefix}
    , numDigits {::numDigits(numImages)}
    , extension {extension}
{

}


std::string ImageNameFormatter::getImageName(std::size_t imageIdx) const
{
    std::string result;

    if (!prefix.empty())
        result += prefix + '_';

    result += str::format("%0*zu", numDigits, imageIdx);
    result += extension;

    return result;
}
