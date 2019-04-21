cmake_minimum_required(VERSION 2.8.12)

project(tests)

add_executable(
    tests

    main.cpp
    test_byteorder.cpp
    test_cp_range.cpp
    test_kerning.cpp
    test_sfnt.cpp
    test_streams.cpp
    test_unicode.cpp
    utils.cpp

    ../src/cp_range.cpp
    ../src/font_renderer/font_renderer.cpp
    ../src/font_renderer/ft_font_renderer.cpp
    ../src/font_renderer/stb_font_renderer.cpp
    ../src/kerning.cpp
    ../src/image.cpp
    ../src/sfnt.cpp
    ../src/str.cpp
    ../src/streams/const_mem_stream.cpp
    ../src/streams/file_stream.cpp
    ../src/streams/stream.cpp
    ../src/unicode.cpp
)

target_include_directories(tests PRIVATE ../src ../src/external)

set_target_properties(
    tests
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
        tests PRIVATE -std=c++11 -Wall -Wextra -pedantic
    )
endif()

target_compile_definitions(
    tests
    PRIVATE
    DPFB_USE_FREETYPE=$<BOOL:${DPFB_USE_FREETYPE}>
    DPFB_USE_STBTT=$<BOOL:${DPFB_USE_STBTT}>
)

if (DPFB_USE_FREETYPE)
    find_package(Freetype REQUIRED)
    target_include_directories(tests PRIVATE ${FREETYPE_INCLUDE_DIRS})
    target_link_libraries(tests ${FREETYPE_LIBRARIES})
endif()
