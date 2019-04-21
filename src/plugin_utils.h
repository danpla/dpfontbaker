
#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>


#define LINK_PLUGIN(T) \
    if (find(getName())) { \
        std::fprintf( \
            stderr, #T " \"%s\" is already registered\n", getName()); \
        std::exit(EXIT_FAILURE); \
    } \
\
    auto** pos = &T::list; \
    while (*pos && std::strcmp((*pos)->name, getName()) < 0) \
        pos = &(*pos)->next; \
\
    next = *pos; \
    *pos = this


template<typename T>
inline const T* findPlugin(const char* name)
{
    for (const auto* p = T::getFirst(); p; p = p->getNext())
        if (std::strcmp(name, p->getName()) == 0)
            return p;

    return nullptr;
}
