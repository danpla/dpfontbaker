
#pragma once

#include "streams/stream.h"


namespace dpfb {
namespace streams {


class ConstMemStream: public Stream {
public:
    ConstMemStream(const void* data, std::size_t dataSize);

    const void* getData() const;

    std::size_t write(const void* src, std::size_t srcSize) noexcept override;
    void writeBuffer(const void* src, std::size_t srcSize) override;

    std::size_t read(void* dst, std::size_t dstSize) noexcept override;

    std::int64_t getSize() const override;
    void seek(std::int64_t offset, SeekOrigin origin) override;
    std::int64_t getPosition() const override;
private:
    const std::uint8_t* data;
    std::size_t dataSize;
    std::size_t pos;
};


}
}
