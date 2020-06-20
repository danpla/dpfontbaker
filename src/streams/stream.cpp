
#include "streams/stream.h"

#include <cstring>


namespace dpfb {
namespace streams {


void Stream::writeBuffer(const void* data, std::size_t dataSize)
{
    if (write(data, dataSize) != dataSize)
        throw StreamError("Write error");
}


void Stream::readBuffer(void* data, std::size_t dataSize)
{
    if (read(data, dataSize) != dataSize)
        throw StreamError("Read error or EOF");
}


std::size_t Stream::writeStr(const char* str) noexcept
{
    return write(str, std::strlen(str));
}


std::size_t Stream::writeStr(const std::string& str) noexcept
{
    return write(str.data(), str.size());
}


}
}
