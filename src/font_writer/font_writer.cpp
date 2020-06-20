
#include "font_writer/font_writer.h"

#include "plugin_utils.h"
#include "str.h"


namespace dpfb {


bool FontWriter::exists(const char* name)
{
    return find(name);
}


const FontWriter& FontWriter::get(const char* name)
{
    if (const auto* writer = find(name))
        return *writer;

    throw FontWriterError(
        str::format("No such font writer: \"%s\"", name));
}


const FontWriter* FontWriter::getFirst()
{
    return list;
}


const FontWriter* FontWriter::getNext() const
{
    return next;
}


FontWriter* FontWriter::list;


FontWriter::FontWriter(const char* name, const char* fileExtension)
    : name {name}
    , fileExtension {fileExtension}
{
    LINK_PLUGIN(FontWriter);
}


const char* FontWriter::getName() const
{
    return name;
}


const char* FontWriter::getFileExtension() const
{
    return fileExtension;
}


const FontWriter* FontWriter::find(const char* name)
{
    return findPlugin<FontWriter>(name);
}


}

