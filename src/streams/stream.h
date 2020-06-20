
#pragma once

#include <cstdint>
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <string>

#include "byteorder.h"


namespace dpfb {
namespace streams {


class StreamError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};


enum class SeekOrigin {
    set,  ///< Seek from the beginning of the stream
    cur,  ///< Seek relative to the current position
    end  ///< Seek relative to the end of the stream
};


class Stream {
public:
    Stream() = default;
    virtual ~Stream() {};

    Stream(const Stream& other) = delete;
    Stream& operator=(const Stream& other) = delete;

    Stream(Stream&& other) = default;
    Stream& operator=(Stream&& other) = default;

    /**
     * Write up to the given number of bytes to the stream.
     *
     * The number of bytes written may be less than srcSize if,
     * for example, there is insufficient space on the underlying
     * physical medium.
     *
     * \param src data to write
     * \param srcSize number of bytes to write
     *
     * \returns the number of bytes written, or 0 on error
     */
    virtual std::size_t write(
        const void* src, std::size_t srcSize) noexcept = 0;

    /**
     * Write the given number of bytes to the stream.
     *
     * The method uses write() and raises StreamError when it writes
     * less than srcSize bytes. You can override this method to
     * provide a more detailed exception message.
     *
     * \param src data to write
     * \param srcSize number of bytes to write
     *
     * \throws StreamError
     */
    virtual void writeBuffer(const void* src, std::size_t srcSize);

    /**
     * Read up to the given number of bytes from the stream.
     *
     * \param dst destination buffer
     * \param dstSize number of bytes to read
     *
     * \returns the number of bytes read, or 0 on an error or EOF
     */
    virtual std::size_t read(void* dst, std::size_t dstSize) noexcept = 0;

    /**
     * Read the given number of bytes from the stream.
     *
     * The method uses read() and throws StreamError when it reads
     * less than dstSize bytes. You can override this method to
     * provide a more detailed exception message.
     *
     * \param dst destination buffer
     * \param dstSize size of the destination buffer
     *
     * \throws StreamError
     */
    virtual void readBuffer(void* dst, std::size_t dstSize);

    /**
     * Return the size of the stream.
     *
     * \throws StreamError if the size is unknown
     */
    virtual std::int64_t getSize() const = 0;

    /**
     * Seek to offset relative to origin.
     *
     * \throws StreamError seeking error, in particular, if the stream
     *     does not support seeking or the given origin
     */
    virtual void seek(std::int64_t offset, SeekOrigin origin) = 0;

    /**
     * Return the current stream position.
     */
    virtual std::int64_t getPosition() const = 0;

    /**
     * Write a null-terminated string.
     *
     * \note The null character will not be written.
     *
     * \returns the number of characters written
     */
    std::size_t writeStr(const char* str) noexcept;

    /**
     * Write a std::string.
     *
     * \returns the number of characters written
     */
    std::size_t writeStr(const std::string& str) noexcept;

    std::uint8_t readU8();
    std::int8_t readS8();

    std::uint16_t readU16Le();
    std::uint32_t readU32Le();
    std::uint16_t readU16Be();
    std::uint32_t readU32Be();

    std::int16_t readS16Le();
    std::int32_t readS32Le();
    std::int16_t readS16Be();
    std::int32_t readS32Be();

    void writeU8(std::uint8_t value);
    void writeS8(std::int8_t value);

    void writeU16Le(std::uint16_t value);
    void writeU32Le(std::uint32_t value);
    void writeU16Be(std::uint16_t value);
    void writeU32Be(std::uint32_t value);

    void writeS16Le(std::int16_t value);
    void writeS32Le(std::int32_t value);
    void writeS16Be(std::int16_t value);
    void writeS32Be(std::int32_t value);
};


inline std::uint8_t Stream::readU8()
{
    std::uint8_t value;
    readBuffer(&value, sizeof(value));
    return value;
}


inline std::int8_t Stream::readS8()
{
    return static_cast<std::int8_t>(readU8());
}


inline std::uint16_t Stream::readU16Le()
{
    std::uint16_t value;
    readBuffer(&value, sizeof(value));
    return byteorder::fromLe(value);
}


inline std::uint32_t Stream::readU32Le()
{
    std::uint32_t value;
    readBuffer(&value, sizeof(value));
    return byteorder::fromLe(value);
}


inline std::uint16_t Stream::readU16Be()
{
    std::uint16_t value;
    readBuffer(&value, sizeof(value));
    return byteorder::fromBe(value);
}


inline std::uint32_t Stream::readU32Be()
{
    std::uint32_t value;
    readBuffer(&value, sizeof(value));
    return byteorder::fromBe(value);
}


inline std::int16_t Stream::readS16Le()
{
    return static_cast<std::int16_t>(readU16Le());
}


inline std::int32_t Stream::readS32Le()
{
    return static_cast<std::int32_t>(readU32Le());
}


inline std::int16_t Stream::readS16Be()
{
    return static_cast<std::int16_t>(readU16Be());
}


inline std::int32_t Stream::readS32Be()
{
    return static_cast<std::int32_t>(readU32Be());
}


inline void Stream::writeU8(std::uint8_t value)
{
    writeBuffer(&value, sizeof(value));
}


inline void Stream::writeS8(std::int8_t value)
{
    writeU8(static_cast<std::uint8_t>(value));
}


inline void Stream::writeU16Le(std::uint16_t value)
{
    value = byteorder::toLe(value);
    writeBuffer(&value, sizeof(value));
}


inline void Stream::writeU32Le(std::uint32_t value)
{
    value = byteorder::toLe(value);
    writeBuffer(&value, sizeof(value));
}


inline void Stream::writeU16Be(std::uint16_t value)
{
    value = byteorder::toBe(value);
    writeBuffer(&value, sizeof(value));
}


inline void Stream::writeU32Be(std::uint32_t value)
{
    value = byteorder::toBe(value);
    writeBuffer(&value, sizeof(value));
}


inline void Stream::writeS16Le(std::int16_t value)
{
    writeU16Le(static_cast<std::uint16_t>(value));
}


inline void Stream::writeS32Le(std::int32_t value)
{
    writeU32Le(static_cast<std::uint32_t>(value));
}


inline void Stream::writeS16Be(std::int16_t value)
{
    writeU16Be(static_cast<std::uint16_t>(value));
}


inline void Stream::writeS32Be(std::int32_t value)
{
    writeU32Be(static_cast<std::uint32_t>(value));
}


}
}
