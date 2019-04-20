
#include "image_writer/image_writer.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

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
    return writers;
}


const ImageWriter* ImageWriter::getNext() const
{
    return next;
}


ImageWriter* ImageWriter::writers;


ImageWriter::ImageWriter(const char* name, const char* fileExtension)
    : name {name}
    , fileExtension {fileExtension}
{
    assert(name);
    assert(fileExtension);

    if (find(name)) {
        std::fprintf(
            stderr, "ImageWriter \"%s\" is already registered\n", name);
        std::exit(EXIT_FAILURE);
    }

    auto** pos = &writers;
    while (*pos && std::strcmp((*pos)->name, name) < 0)
        pos = &(*pos)->next;

    next = *pos;
    *pos = this;
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
    for (auto* writer = writers; writer; writer = writer->next)
        if (std::strcmp(name, writer->name) == 0)
            return writer;

    return nullptr;
}
