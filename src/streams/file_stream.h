
#pragma once

#include <cstdio>

#include "streams/stream.h"


namespace dpfb {
namespace streams {


class FileStream : public Stream {
public:
    FileStream(const char* fileName, const char* mode);
    FileStream(const std::string& fileName, const char* mode);
    ~FileStream();

    FileStream(FileStream&& other);
    FileStream& operator=(FileStream&& other);

    std::size_t write(const void* src, std::size_t srcSize) noexcept override;
    void writeBuffer(const void* src, std::size_t srcSize) override;

    std::size_t read(void* dst, std::size_t dstSize) noexcept override;
    void readBuffer(void* dst, std::size_t dstSize) override;

    std::int64_t getSize() const override;
    void seek(std::int64_t offset, SeekOrigin origin) override;
    std::int64_t getPosition() const override;
private:
    std::FILE* fp;
};


}
}
