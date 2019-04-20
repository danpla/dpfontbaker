
#include "font_renderer/font_renderer.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "str.h"


bool FontRenderer::exists(const char* name)
{
    return FontRendererCreator::find(name);
}


FontRenderer* FontRenderer::create(
    const char* name, const FontRendererArgs& args)
{
    if (const auto* creator = FontRendererCreator::find(name))
        return creator->create(args);

    throw FontRendererError(
        str::format("No such font renderer: \"%s\"", name));
}


FontRendererCreator* FontRendererCreator::creators;


const FontRendererCreator* FontRendererCreator::find(const char* name)
{
    for (auto* creator = creators; creator; creator = creator->next)
        if (std::strcmp(name, creator->getName()) == 0)
            return creator;

    return nullptr;
}


const FontRendererCreator* FontRendererCreator::getFirst()
{
    return creators;
}


const FontRendererCreator* FontRendererCreator::getNext() const
{
    return next;
}


FontRendererCreator::FontRendererCreator(const char* name)
    : name {name}
{
    assert(name);

    if (find(name)) {
        std::fprintf(
            stderr, "FontRenderer \"%s\" is already registered\n", name);
        std::exit(EXIT_FAILURE);
    }

    auto** pos = &creators;
    while (*pos && std::strcmp((*pos)->name, name) < 0)
        pos = &(*pos)->next;

    next = *pos;
    *pos = this;
}


const char* FontRendererCreator::getName() const
{
    return name;
}
