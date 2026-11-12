// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/port/track/topology.hpp"
#include <optional>
#include "game_core/port/driving/memory.hpp"
#include "game_core/port/driving/object_view.hpp"
#include "game_core/port/track/topology_resources.hpp"
namespace hp2::debug {
std::uint32_t GetTrackInfo(const ::port::driving::Memory&,int); //00153070, W72; source Name/range gates.
enum eConstraintType {eCONSTRAIN_BY_ROTATION=0,eCONSTRAIN_BY_SCALE=1}; // E-0099-W50, captured original enum.
class TrackPolygon:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 bool PointInShaft(const std::array<float,4>&,float* elevation=nullptr); //00151428.
 bool SegmentIntersects(const std::array<float,4>&,const std::array<float,4>&,std::array<float,4>* intersection); //001515E0.
 bool TestSegmentToPoly(float& nearest,const std::array<float,4>& target,const std::array<float,4>& position,const std::array<float,4>& low,const std::array<float,4>& high,bool two_sided); //001516D8.
};
class TopologyCoordinate : public ::port::driving::ObjectView {
public:
    using ObjectView::ObjectView;
    // Debug00151878/00151920/00151980/00151CC0; E-0099-W26.
    void Sub_00151878();
    void Sub_001518C0(int in_chrg); // Debug001518C0; releases references, no field resets.
    void ClearInterestBBox();
    void SetInterestBBox(const std::array<float,2>&,float,const std::array<float,2>* velocity=nullptr);
    float GetElevation(const std::array<float,3>&,int* terrain=nullptr,std::array<float,4>* normal=nullptr,bool* valid=nullptr);
    bool HasTopology(const std::array<float,2>&); //Debug001521A8, shared cache; E-0099-W39.
    bool ConstrainToTopology(eConstraintType,bool two_sided,const std::array<float,4>& target,std::array<float,4>& position,float wall_margin,float wall_up_max); //00152500; E-0099-W50.
    // Debug001521A8 ->00151CC0, E-0099-W17. Fresh-coordinate branch only;
    // original test elevation is read from the authorized Debug image.
    static bool HasTopology(const ::port::driving::Memory&,const ::port::track::Topology&,const std::array<float,2>&);
};
class TopologyTree : public ::port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 // E-0099-W26, original track.cpp; source-backed pointers throughout.
 void Sub_00152760();
 void InitTree(); //00152798
 void CloseTree(); //001527E8
 std::uint32_t GetSquare(float,float); //00152A58, return_closest_square=0
 std::vector<std::uint32_t> BuildTrackPolygons(const std::array<float,2>&,const std::array<float,2>&,int maximum); //00152E48
 void RemoveTrackPolygons(std::uint32_t table,int count); //00153018
 std::uint32_t AssignTrackPolygon(std::uint32_t); //00152D98
 void UnAssignTrackPolygon(std::uint32_t); //00152E38
 void FlushUnusedTrackPolygons(); //00152CF8
};
}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class TopologyCoordinate {
public:
    // ELF 0x00152070; E-0042-C021; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/00152070.md.
    // Original Debug reference: TopologyCoordinate::GetElevation(bVector3 const *, TerrainType *, bVector3 *, bool *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetElevation();
    // Retail 0x00152070; E-0048-T01. Fresh candidate selection host binding.
    static port::track::Contact GetElevation(const port::track::Topology&, const std::array<float,3>&);
};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class TopologyTree {
public:
    // ELF 0x00152B10; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00152b10.md.
    // Original Debug reference: TopologyTree::TopologyTree(void); constructor/destructor entry is not a native C++ lifetime operation.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_00152B10();
    // ELF 0x00152C10; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00152c10.md.
    // Original Debug reference: TopologyTree::Loader(bChunk *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Loader();
    // Retail 0x00152C10; E-0048-T01. Host-owned chunk binding.
    static bool Loader(port::track::Topology&,std::uint32_t,const std::vector<std::uint8_t>&,std::size_t,std::size_t);
    // ELF 0x00152D30; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00152d30.md.
    // Original Debug reference: TopologyTree::Unloader(bChunk *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Unloader();
    // Retail 0x00152D30; E-0048-T01. Host-owned chunk binding.
    static bool Unloader(port::track::Topology&,std::uint32_t);
    // ELF 0x00152DB0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00152db0.md.
    // Original Debug reference: TopologyTree::GetSquareCorner(float); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetSquareCorner();
    // Retail 0x00152DB0; E-0048-T01.
    static float GetSquareCorner(float);
    // ELF 0x00152E08; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00152e08.md.
    // Original Debug reference: TopologyTree::GetSquare(float, float, int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetSquare();
    // Retail 0x00152E08; E-0048-T01. No nearest-square fallback in this binding.
    static std::optional<std::size_t> GetSquare(const port::track::Topology&,float,float);
    // ELF 0x00152F38; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00152f38.md.
    // Original Debug reference: TopologyTree::GetIntersectingPolygons(TopologyTreePolygon **, int, TopologyTreeSquare *, bVector2 const *, bVector2 const *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetIntersectingPolygons();
    // Retail 0x001531F8; E-0048-T01. Original BuildTrackPolygons host storage binding.
    static std::vector<std::size_t> BuildTrackPolygons(const port::track::Topology&,const std::array<float,2>&,const std::array<float,2>&,std::size_t);
};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class TrackPolygon {
public:
    // ELF 0x001517D8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001517d8.md.
    // Original Debug reference: TrackPolygon::PointInShaft(bVector3 *, float *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void PointInShaft();
    // Retail 0x00151990; E-0048-T02; Debug correspondence 0x001515E0.
    static bool SegmentIntersects(const port::track::Polygon&,const std::array<float,3>&,const std::array<float,3>&,std::array<float,3>*);
    // Retail 0x001514A8; E-0048-T01. Packed-polygon expansion; host storage only.
    static port::track::Polygon Sub_001514A8(const std::uint8_t*);
    // Retail 0x001517D8; E-0048-T01.
    static bool PointInShaft(const port::track::Polygon&,const std::array<float,3>&,float*);
};
} // namespace hp2::recovered
