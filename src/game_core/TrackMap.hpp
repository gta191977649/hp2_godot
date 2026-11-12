// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class TrackMap {
public:
    // No member address is confirmed; no speculative members are declared.

public:
    // TASK-0028: missing entries; existing storage and bodies are preserved.
    // ELF 0x001BCA08; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001bca08.md.
    // Original Debug reference: TrackMap::EmitPoint(bVector2 *, bVector3 *, unsigned char, bVector2 *, bVector3 *, unsigned char *, int *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void EmitPoint();
};
} // namespace hp2::recovered

#include "game_core/port/driving/object_view.hpp"
#include "game_core/port/race/hud_geometry.hpp"
#include <vector>
namespace hp2::debug {
// E-0112-M01. Original0xB0-byte logical-memory TrackMap view.
class TrackMap:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 void Sub_001B7FD0(std::uint32_t track,std::uint32_t car,std::uint32_t player);
 void Sub_001B81E0(int flags);
 void InitTrackMapModeInfo(); //001BA610.
 void Render(std::vector<::port::race::HudPolygon>&); //001B8200.
 int GetAngle(std::uint16_t&); //001B8A90.
 int GetCameraDirection(std::array<float,4>&); //001BA4C8.
 std::array<float,2> GetCarUVPosition(const std::array<float,4>&); //001B9E68.
 std::array<std::array<float,2>,4> GetBoxOnMap(const std::array<float,2>&,float,float,float,float,std::uint16_t); //001B9F00.
 void MapToScreen(::port::race::HudPolygon&,std::array<std::array<float,4>,4>&); //001BA070.
 void ClipPolygon(const ::port::race::HudPolygon&,std::uint32_t texture,int layer,std::vector<::port::race::HudPolygon>&); //001B9558.
 void DrawMap(std::array<float,2>&,std::array<std::array<float,2>,4>&,std::uint16_t,std::vector<::port::race::HudPolygon>&); //001B8BC0.
 void DrawMarker(const std::array<float,4>&,const std::array<float,2>&,std::uint16_t,const std::array<std::array<float,2>,4>&,std::uint32_t,int,std::uint32_t,std::uint16_t,float,float,std::vector<::port::race::HudPolygon>&); //001B9068.
 void DrawOverOrUnderLay(std::uint32_t,int,std::vector<::port::race::HudPolygon>&); //001B93A0.
};
}
