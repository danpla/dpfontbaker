
#include "font_writer/font_writer.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "str.h"


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
    return writers;
}


const FontWriter* FontWriter::getNext() const
{
    return next;
}


FontWriter* FontWriter::writers;


FontWriter::FontWriter(const char* name, const char* fileExtension)
    : name {name}
    , fileExtension {fileExtension}
{
    assert(name);
    assert(fileExtension);

    if (find(name)) {
        std::fprintf(
            stderr, "FontWriter \"%s\" is already registered\n", name);
        std::exit(EXIT_FAILURE);
    }

    auto** pos = &writers;
    while (*pos && std::strcmp((*pos)->name, name) < 0)
        pos = &(*pos)->next;

    next = *pos;
    *pos = this;
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
    for (auto* writer = writers; writer; writer = writer->next)
        if (std::strcmp(name, writer->name) == 0)
            return writer;

    return nullptr;
}

