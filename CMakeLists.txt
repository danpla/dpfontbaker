cmake_minimum_required(VERSION 2.8.12)

project(dpFontBaker)

option(DPFB_USE_FREETYPE "Include FreeType renderer" ON)
option(DPFB_USE_STBTT "Include stb_truetype.h renderer" ON)
option(DPFB_USE_LIBPNG "Enable PNG support" ON)
option(DPFB_BUILD_TESTS "Build unit tests" OFF)

add_executable(
    dpfb

    src/args.cpp
    src/cp_range.cpp
    src/font.cpp
    src/font_renderer/font_renderer.cpp
    src/font_renderer/ft_font_renderer.cpp
    src/font_renderer/stb_font_renderer.cpp
    src/font_writer/bmfont_writer.cpp
    src/font_writer/font_writer.cpp
    src/font_writer/json_font_writer.cpp
    src/image.cpp
    src/image_name_formatter.cpp
    src/image_writer/image_writer.cpp
    src/image_writer/pgm_image_writer.cpp
    src/image_writer/png_image_writer.cpp
    src/image_writer/tga_image_writer.cpp
    src/kerning.cpp
    src/main.cpp
    src/sfnt.cpp
    src/str.cpp
    src/streams/const_mem_stream.cpp
    src/streams/file_stream.cpp
    src/streams/stream.cpp
    src/unicode.cpp
    src/version.cpp
)

target_include_directories(
    dpfb
    PRIVATE
    src src/external
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
        dpfb PRIVATE -std=c++11 -Wall -Wextra -pedantic
    )
endif()

target_compile_definitions(
    dpfb
    PRIVATE
    DPFB_USE_FREETYPE=$<BOOL:${DPFB_USE_FREETYPE}>
    DPFB_USE_STBTT=$<BOOL:${DPFB_USE_STBTT}>
    DPFB_USE_LIBPNG=$<BOOL:${DPFB_USE_LIBPNG}>
)

if (DPFB_USE_FREETYPE)
    find_package(Freetype REQUIRED)
    target_include_directories(dpfb PRIVATE ${FREETYPE_INCLUDE_DIRS})
    target_link_libraries(dpfb ${FREETYPE_LIBRARIES})
endif()

if (DPFB_USE_STBTT)
    # Extract version
    file(STRINGS src/external/stb_truetype.h LINE LIMIT_COUNT 1)
    string(
        REGEX MATCH
        "// stb_truetype.h - v(.*) - public domain"
        _
        ${LINE}
    )

    if (NOT ${CMAKE_MATCH_1} STREQUAL "")
        target_compile_definitions(
            dpfb PRIVATE STB_TRUETYPE_H_VERSION="${CMAKE_MATCH_1}"
        )
    else()
        message(WARNING "Can't extract stb_truetype.h version")
    endif()
endif()

if (DPFB_USE_LIBPNG)
    find_package(PNG REQUIRED)
    target_include_directories(dpfb PRIVATE ${PNG_INCLUDE_DIRS})
    target_link_libraries(dpfb ${PNG_LIBRARIES})
endif()

if (DPFB_BUILD_TESTS)
    add_subdirectory(tests)
endif()
