
#include "font_renderer/font_renderer.h"

#include "plugin_utils.h"
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


FontRendererCreator* FontRendererCreator::list;


const FontRendererCreator* FontRendererCreator::find(const char* name)
{
    return findPlugin<FontRendererCreator>(name);
}


const FontRendererCreator* FontRendererCreator::getFirst()
{
    return list;
}


const FontRendererCreator* FontRendererCreator::getNext() const
{
    return next;
}


FontRendererCreator::FontRendererCreator(const char* name)
    : name {name}
{
    LINK_PLUGIN(FontRendererCreator);
}


const char* FontRendererCreator::getName() const
{
    return name;
}
