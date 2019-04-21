
#pragma once

#include <stdexcept>

#include "image.h"
#include "streams/stream.h"


class ImageWriterError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};


class ImageWriter {
public:
    static bool exists(const char* name);
    static const ImageWriter& get(const char* name);

    static const ImageWriter* getFirst();
    const ImageWriter* getNext() const;

    ImageWriter(const char* name, const char* fileExtension);
    virtual ~ImageWriter() {};

    ImageWriter(const ImageWriter& other) = delete;
    ImageWriter& operator=(const ImageWriter& other) = delete;

    ImageWriter(ImageWriter&& other) = delete;
    ImageWriter& operator=(ImageWriter&& other) = delete;

    const char* getName() const;
    const char* getFileExtension() const;
    virtual const char* getDescription() const = 0;

    virtual void write(
        streams::Stream& stream,
        const Image& image) const = 0;
private:
    static ImageWriter* list;

    const char* name;
    const char* fileExtension;
    ImageWriter* next;

    static const ImageWriter* find(const char* name);
};
