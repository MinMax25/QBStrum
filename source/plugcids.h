//------------------------------------------------------------------------
// Copyright(c) 2025 MinMax.
//------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

namespace MinMax {
//------------------------------------------------------------------------
static const Steinberg::FUID kMyVSTProcessorUID (0x935FDF30, 0x800A5DD4, 0xA8D2329D, 0xD97A3B7C);
static const Steinberg::FUID kMyVSTControllerUID (0xCFB370C0, 0x98085FE5, 0x91657B14, 0x3F284F39);

#define MyVSTVST3Category "Instrument"

//------------------------------------------------------------------------
} // namespace MinMax
