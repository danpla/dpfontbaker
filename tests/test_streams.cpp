
#include <cstdio>
#include <cstring>

#include "catch.hpp"

#include "streams/const_mem_stream.h"
#include "streams/file_stream.h"


using namespace streams;


const char testBuf[] =
    "Lorem ipsum dolor sit amet, consectetur adipisicing elit, "
    "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
    "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris "
    "nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in "
    "reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla "
    "pariatur. Excepteur sint occaecat cupidatat non proident, sunt in "
    "culpa qui officia deserunt mollit anim id est laborum.";


static void commonWrite(Stream& stream)
{
    REQUIRE(stream.getPosition() == 0);
    REQUIRE(stream.getSize() == 0);

    REQUIRE_THROWS_AS(stream.readS8(), StreamError);
    REQUIRE(stream.getPosition() == 0);

    // Can seek past end
    REQUIRE_NOTHROW(stream.seek(4, SeekOrigin::set));
    REQUIRE(stream.getPosition() == 4);
    REQUIRE(stream.getSize() == 0);

    REQUIRE_NOTHROW(stream.writeU8(0));
    REQUIRE(stream.getPosition() == 5);
    REQUIRE(stream.getSize() == 5);

    REQUIRE_NOTHROW(stream.seek(0, SeekOrigin::set));
    REQUIRE(stream.getPosition() == 0);
    REQUIRE(stream.getSize() == 5);

    REQUIRE(stream.write(testBuf, sizeof(testBuf)) == sizeof(testBuf));
    REQUIRE(stream.getPosition() == sizeof(testBuf));
    REQUIRE(stream.getSize() == sizeof(testBuf));
}


static void commonRead(Stream& stream)
{
    REQUIRE(stream.getPosition() == 0);
    REQUIRE(stream.getSize() == sizeof(testBuf));

    REQUIRE_THROWS_AS(stream.writeS8(0), StreamError);
    REQUIRE(stream.getPosition() == 0);

    REQUIRE_THROWS_AS(stream.seek(-1, SeekOrigin::cur), StreamError);
    REQUIRE(stream.getPosition() == 0);

    // Can seek past end
    REQUIRE_NOTHROW(stream.seek(stream.getSize() + 100, SeekOrigin::set));
    REQUIRE(stream.getPosition() == stream.getSize() + 100);

    REQUIRE_THROWS_AS(stream.readU8(), StreamError);
    REQUIRE(stream.getPosition() == stream.getSize() + 100);

    REQUIRE_NOTHROW(stream.seek(0, SeekOrigin::set));
    REQUIRE(stream.getPosition() == 0);

    char inBuf[sizeof(testBuf)];
    REQUIRE(stream.read(inBuf, sizeof(inBuf)) == sizeof(inBuf));
    REQUIRE(std::memcmp(testBuf, inBuf, sizeof(inBuf)) == 0);
    REQUIRE(stream.getPosition() == sizeof(inBuf));
}


TEST_CASE("FileStream", "[streams]") {
    const char* tmpFile = "file.bin";

    SECTION("Writing") {
        FileStream stream(tmpFile, "wb");
        commonWrite(stream);
    }

    SECTION("Reading") {
        // On Windows, the file must be closed before remove(),
        // so put it in a separate scope.
        {
            FileStream stream(tmpFile, "rb");
            commonRead(stream);
        }

        std::remove(tmpFile);
    }
}


TEST_CASE("ConstMemStream", "[streams]") {
    REQUIRE_THROWS_AS(ConstMemStream(nullptr, 4), StreamError);
    REQUIRE_NOTHROW(ConstMemStream(testBuf, 0));

    SECTION("Writing") {
        ConstMemStream stream(testBuf, sizeof(testBuf));
        REQUIRE(stream.getPosition() == 0);
        REQUIRE(stream.getSize() == sizeof(testBuf));

        REQUIRE_THROWS_AS(stream.writeU8(0), StreamError);
        REQUIRE(stream.getPosition() == 0);
    }

    SECTION("Reading") {
        ConstMemStream stream(testBuf, sizeof(testBuf));
        commonRead(stream);
    }
}
