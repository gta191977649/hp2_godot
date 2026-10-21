#pragma once
#include "game_core/port/track/postprocess.hpp"
namespace hp2::recovered {
// Original declaring TU src/ecstasy/eenveffectse.cpp, E-0042-S001.
// TASK0052 typed immutable carriers, not original eView/eDmaChain ABI.
port::track::postprocess::PaletteResult SetupBlackAndWhite(); // Retail11A990/Debug11AD58
void RemapLinearIntoClut8(port::track::postprocess::Palette&,const port::track::postprocess::Palette&); // Retail1189F8/Debug118DC0
unsigned MapScreenZToPaletteIndex(float); // Retail119B58/Debug119F20
float MapPaletteIndexToScreenZ(int); // Retail119B70/Debug119F38
float MapScreenZToViewZ(float,float,float); // Retail119B90/Debug119F58
port::track::postprocess::PaletteResult CreateFogPalette(const port::track::postprocess::View&,const port::track::postprocess::Controls&); // Retail119308/Debug1196D0
port::track::postprocess::PaletteResult CreateFocusPalette(const port::track::postprocess::View&,const port::track::postprocess::Controls&); // Retail119C20/Debug119FE8
}

namespace hp2::recovered {
// Retail00118A60/Debug00118E28; E-0052-P01. Semantic three-copy projection.
bool eAttachDepthIntoAlpha(const port::track::postprocess::View&,const port::track::postprocess::Controls&,port::track::postprocess::Frame&,unsigned,unsigned);
// Retail00119768/Debug00119B30; E-0052-P01. Immutable per-view attachment slice.
bool eAttachFog(const port::track::postprocess::View&,const port::track::postprocess::Controls&,port::track::postprocess::Frame&,unsigned,unsigned);
// Retail00119F70/Debug0011A338; E-0052-P02. Original half-size two-jitter route.
bool eAttachDepthOfField(const port::track::postprocess::View&,const port::track::postprocess::Controls&,port::track::postprocess::Frame&,unsigned,unsigned);
}

namespace hp2::recovered {
// Retail0011A9F0/Debug0011ADB8. Original CT16 mask/PSMT8H palette attachment.
bool eAttachBlackAndWhite(const port::track::postprocess::View&,const port::track::postprocess::Controls&,port::track::postprocess::Frame&,unsigned,unsigned);
// Per-pixel semantic projection of that packet route; explicit MainDisp0 A before the BW copy.
std::uint32_t eAttachBlackAndWhite(std::uint32_t current_rgba,std::uint8_t main_display_alpha);
}
