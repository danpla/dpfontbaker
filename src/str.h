
#pragma once

#include <string>


namespace str {


std::string format(const char* fmt, ...)
    #ifdef __GNUC__
    __attribute__((format(printf, 1, 2)));
    #endif


}
