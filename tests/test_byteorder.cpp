
#include "catch.hpp"

#include "byteorder.h"


using namespace byteorder;


TEST_CASE("swap", "[byteorder]") {
    REQUIRE(swap(std::uint16_t(0x1234)) == 0x3412);
    REQUIRE(swap(std::uint32_t(0x12345678ul)) == 0x78563412);
}

