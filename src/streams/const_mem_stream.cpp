
#include "streams/const_mem_stream.h"

#include <cstring>


namespace dpfb {
namespace streams {


ConstMemStream::ConstMemStream(const void* data, std::size_t dataSize)
    : data {static_cast<const std::uint8_t*>(data)}
    , dataSize {dataSize}
    , pos {0}
{
    if (!data)
        throw StreamError("data is nullptr");
}


const void* ConstMemStream::getData() const
{
    return data;
}


std::size_t ConstMemStream::write(const void*, std::size_t) noexcept
{
    return 0;
}


void ConstMemStream::writeBuffer(const void*, std::size_t)
{
    throw StreamError("ConstMemStream is read-only");
}


std::size_t ConstMemStream::read(void* dst, std::size_t dstSize) noexcept
{
    if (!dst || dstSize == 0 || pos >= dataSize)
        return 0;

    std::size_t maxRead = dataSize - pos;
    if (dstSize > maxRead)
        dstSize = maxRead;

    std::memcpy(dst, data + pos, dstSize);
    pos += dstSize;

    return dstSize;
}


std::int64_t ConstMemStream::getSize() const
{
    return dataSize;
}


void ConstMemStream::seek(std::int64_t offset, SeekOrigin origin)
{
    switch (origin) {
        case SeekOrigin::set:
            break;
        case SeekOrigin::cur:
            offset += pos;
            break;
        case SeekOrigin::end:
            offset += dataSize;
            break;
    }

    if (offset < 0)
        throw StreamError("Offset points before the beginning");

    pos = offset;
}


std::int64_t ConstMemStream::getPosition() const
{
    return pos;
}


}
}
