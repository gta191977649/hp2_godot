#pragma once
// Port-owned lifetime of raw original track-marker chunks; E-0099-W34.
#include "game_core/port/driving/memory.hpp"
#include <vector>
namespace hp2::debug {
// Debug00151018/00151058, original indep/src/track.cpp.
int GetNumTrackPositionMarkers(const ::port::driving::Memory&,std::uint32_t hash);
std::uint32_t GetTrackPositionMarker(const ::port::driving::Memory&,std::uint32_t hash,int index);
// Debug00153258/00153368: only the recovered34146 marker branch.
int LoaderTrackGeometry(::port::driving::Memory&,std::uint32_t chunk);
int UnloaderTrackGeometry(::port::driving::Memory&,std::uint32_t chunk);
}
namespace port::track {
struct PositionMarkerResources {
 ::port::driving::Memory* owner=nullptr;
 std::vector<std::uint32_t> chunks;
};
// Original global-list member initializer00153530 only; not whole tree startup.
void initialize_position_marker_list(::port::driving::Memory&);
// Complete authored34146 records. Input is the player's decompressed Retail
// bundle. Retail00153608/0014A020 establish24-byte stride, hash+8/Param+C/XY+10
// correspondence; no whole TrackInfo layout equivalence is asserted.
PositionMarkerResources load_retail_position_markers(::port::driving::Memory&,const std::vector<std::uint8_t>&);
void unload_position_markers(::port::driving::Memory&,PositionMarkerResources&);
}
