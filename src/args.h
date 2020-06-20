
#pragma once


namespace dpfb {
namespace args {


extern const char* fontPath;

extern const char* codePoints;
extern int fontDpi;
extern const char* fontExportFormat;
extern const char* fontExportName;
extern int fontIndex;
extern const char* fontRenderer;
extern int fontSize;
extern int glyphPaddingInner[4];
extern int glyphPaddingOuter[4];
extern int glyphSpacing[2];
extern const char* hinting;
extern const char* imageFormat;
extern int imageMaxCount;
extern int imageMaxSize;
extern int imagePadding[4];
extern const char* imageSizeMode;
extern const char* kerning;
extern const char* outDir;


void parse(int argc, char* argv[]);


}
}
