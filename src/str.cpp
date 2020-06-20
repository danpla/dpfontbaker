
#include "str.h"

#include <cstdio>
#include <cstdarg>


namespace dpfb {
namespace str {


std::string format(const char* fmt, ...)
{
    va_list args1;
    va_start(args1, fmt);
    va_list args2;
    va_copy(args2, args1);

    const auto size = std::vsnprintf(nullptr, 0, fmt, args1);
    va_end(args1);
    if (size <= 0) {
        va_end(args2);
        return "";
    }

    std::string result;
    result.resize(size + 1);
    std::vsnprintf(&result[0], result.length(), fmt, args2);
    va_end(args2);

    result.pop_back();
    return result;
}


}
}
