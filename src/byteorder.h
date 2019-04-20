
// Since this file in the root directory, it's named byteorder.h
// rather than endian.h to avoid conflicts with system-wide endian.h
// on Unix-like systems.
// For examle, see:
//    https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=598600

#pragma once

#include <cstdint>


namespace byteorder {


enum class ByteOrder {
    big,
    little
};


inline ByteOrder getNativeByteOrder()
{
    const unsigned short i = 1;
    if (reinterpret_cast<const unsigned char&>(i) == 1)
        return ByteOrder::little;
    else
        return ByteOrder::big;
}


inline std::uint16_t swap(std::uint16_t n)
{
    return (n << 8) | (n >> 8);
}


inline std::uint32_t swap(std::uint32_t n)
{
    return (
        (n << 24)
        | ((n << 8) & 0x00FF0000)
        | ((n >> 8) & 0x0000FF00)
        | (n >> 24));
}


template<typename T>
inline T swapLe(T t)
{
    if (getNativeByteOrder() == ByteOrder::little)
        return t;
    else
        return swap(t);
}

template<typename T>
inline T swapBe(T t)
{
    if (getNativeByteOrder() == ByteOrder::little)
        return swap(t);
    else
        return t;
}


template<typename T>
inline T toLe(T t)
{
    return swapLe(t);
}


template<typename T>
inline T fromLe(T t)
{
    return swapLe(t);
}


template<typename T>
inline T toBe(T t)
{
    return swapBe(t);
}


template<typename T>
inline T fromBe(T t)
{
    return swapBe(t);
}


}
