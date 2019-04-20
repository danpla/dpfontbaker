
#include "streams/file_stream.h"

#include <cstring>
#include <cerrno>
#include <utility>


namespace streams {


static void throwErrno()
{
    throw StreamError(std::strerror(errno));
}


FileStream::FileStream(const char* fileName, const char* mode)
{
    fp = std::fopen(fileName, mode);
    if (!fp)
        throwErrno();
}


FileStream::FileStream(const std::string& fileName, const char* mode)
    : FileStream(fileName.c_str(), mode)
{

}


FileStream::~FileStream()
{
    if (fp)
        std::fclose(fp);
}


FileStream::FileStream(FileStream&& other)
    : Stream {std::move(other)}
    , fp {other.fp}
{
    other.fp = nullptr;
}


FileStream& FileStream::operator=(FileStream&& other)
{
    if (this != &other) {
        Stream::operator=(std::move(other));

        if (fp)
            std::fclose(fp);

        fp = other.fp;
        other.fp = nullptr;
    }

    return *this;
}


std::size_t FileStream::write(const void* src, std::size_t srcSize) noexcept
{
    return std::fwrite(src, 1, srcSize, fp);
}


void FileStream::writeBuffer(const void* src, std::size_t srcSize)
{
    if (write(src, srcSize) != srcSize)
        throwErrno();
}


std::size_t FileStream::read(void* dst, std::size_t dstSize) noexcept
{
    return std::fread(dst, 1, dstSize, fp);
}


void FileStream::readBuffer(void* dst, std::size_t dstSize)
{
    if (read(dst, dstSize) != dstSize)
        throwErrno();
}


std::int64_t FileStream::getSize() const
{
    const auto pos = getPosition();
    if (std::fseek(fp, 0, SEEK_END) != 0)
        throwErrno();

    const auto size = getPosition();
    if (std::fseek(fp, pos, SEEK_SET) != 0)
        throwErrno();
    return size;
}


void FileStream::seek(std::int64_t offset, SeekOrigin origin)
{
    int whence = SEEK_SET;
    switch (origin) {
        case SeekOrigin::set:
            whence = SEEK_SET;
            break;
        case SeekOrigin::cur:
            whence = SEEK_CUR;
            break;
        case SeekOrigin::end:
            whence = SEEK_END;
            break;
    }

    if (std::fseek(fp, offset, whence) != 0)
        throwErrno();
}


std::int64_t FileStream::getPosition() const
{
    const auto pos = std::ftell(fp);
    if (pos < 0)
        throwErrno();
    return pos;
}


}
