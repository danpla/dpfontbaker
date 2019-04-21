
#include "image_writer/image_writer.h"

#include "plugin_utils.h"
#include "str.h"


bool ImageWriter::exists(const char* name)
{
    return find(name);
}


const ImageWriter& ImageWriter::get(const char* name)
{
    if (const auto* writer = find(name))
        return *writer;

    throw ImageWriterError(
        str::format("No such image writer: \"%s\"", name));
}


const ImageWriter* ImageWriter::getFirst()
{
    return list;
}


const ImageWriter* ImageWriter::getNext() const
{
    return next;
}


ImageWriter* ImageWriter::list;


ImageWriter::ImageWriter(const char* name, const char* fileExtension)
    : name {name}
    , fileExtension {fileExtension}
{
    LINK_PLUGIN(ImageWriter);
}


const char* ImageWriter::getName() const
{
    return name;
}


const char* ImageWriter::getFileExtension() const
{
    return fileExtension;
}


const ImageWriter* ImageWriter::find(const char* name)
{
    return findPlugin<ImageWriter>(name);
}
