// TASK-0042 E-0042-S001: exact Debug source marker(s): ../indep/inc/Scenery.hpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include <vector>
#include <cstddef>
#include <array>

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): GrandSceneryCullInfo::CullView [0x002B00A8]
// Diagnostic text (not a callable signature): GrandSceneryCullInfo::DoCulling() [0x002B00C8]
namespace port::track {struct Scene;struct FrameView;struct Frame;struct Section;}
namespace port::driving {class Memory;}
namespace hp2::recovered {
class ScenerySectionHeader {
public:
    static void TreeCull(const port::track::Scene&,const port::track::Section&,const port::track::FrameView&,port::driving::Memory&,port::track::Frame&);
    static void DrawAScenery(const port::track::Scene&,std::size_t,const port::track::FrameView&,unsigned,port::driving::Memory&,port::track::Frame&);
};
class GrandSceneryCullInfo {
public:
    // Retail00150EB8 / Debug00150B08: static selected draw-info projection.
    static void StuffScenery(const port::track::Scene&,std::size_t instance,std::size_t solid,
        unsigned lod,const port::track::FrameView&,unsigned visibility,
        const std::array<float,16>& matrix,bool identity,float depth,port::track::Frame&);
    static void WhatSectionsShouldWeDraw(const port::track::Scene&,const port::track::FrameView&,std::vector<std::size_t>& zone_order,port::track::Frame&);
    static void CullView(const port::track::Scene&,const port::track::FrameView&,port::driving::Memory&,port::track::Frame&);
    static void DoCulling(const port::track::Scene&,const port::track::FrameView&,std::vector<std::size_t>& zone_order,port::driving::Memory&,port::track::Frame&);
};
} // namespace hp2::recovered
