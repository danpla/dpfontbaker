
#pragma once

#include <vector>

#include "sfnt.h"
#include "streams/stream.h"


namespace dpfb {


struct RawKerningPair {
    std::uint16_t glyphIdx1;
    std::uint16_t glyphIdx2;
    int amount;
};


struct KerningParams {
    int pxSize;
    int pxPerEm;
};


std::vector<RawKerningPair> readKerningPairsKern(
    streams::Stream& stream,
    const SfntOffsetTable& sfntOffsetTable,
    const KerningParams& params);


std::vector<RawKerningPair> readKerningPairsGpos(
    streams::Stream& stream,
    const SfntOffsetTable& sfntOffsetTable,
    const KerningParams& params);


}

