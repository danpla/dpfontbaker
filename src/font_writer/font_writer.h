
#pragma once

#include <stdexcept>

#include "streams/stream.h"
#include "image_name_formatter.h"


class FontWriterError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};


class Font;


class FontWriter {
public:
    static bool exists(const char* name);
    static const FontWriter& get(const char* name);

    static const FontWriter* getFirst();
    const FontWriter* getNext() const;

    FontWriter(const char* name, const char* fileExtension);
    virtual ~FontWriter() {}

    const char* getName() const;
    const char* getFileExtension() const;
    virtual const char* getDescription() const = 0;

    virtual void write(
        streams::Stream& stream,
        const Font& font,
        const ImageNameFormatter& imageNameFormatter) const = 0;
private:
    static FontWriter* writers;

    const char* name;
    const char* fileExtension;
    FontWriter* next;

    static const FontWriter* find(const char* name);
};
