// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/port/track/voronoi.hpp"

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class VCEdge {
public:
    // Debug 0x001CF8F0; E-0048-T03; host feature storage binding.
    static void SetVoronoiPlanes(port::track::VoronoiPolyhedron&,unsigned);
    // ELF 0x001D2A48; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d2a48.md.
    // Original Debug reference: VCEdge::SetVoronoiPlanes(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void SetVoronoiPlanes();
};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class VCFace {
public:
    // Debug 0x001CFC68; E-0048-T03; host feature storage binding.
    static void SetVoronoiPlanes(port::track::VoronoiPolyhedron&,unsigned);
    // ELF 0x001D30A8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d30a8.md.
    // Original Debug reference: VCFace::Move(bVector3 *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Move();
};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class VCPolyhedron {
public:
    // Debug 0x001D01C8/001D0D10; E-0048-T04; memory-image conversion host binding.
    static void MemoryImageLoad(port::track::VoronoiPolyhedron&,const port::track::VoronoiImage&);
    // ELF 0x001D3100; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d3100.md.
    // Original Debug reference: VCPolyhedron::VCPolyhedron(void); constructor/destructor entry is not a native C++ lifetime operation.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001D3100();
    // ELF 0x001D3200; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d3200.md.
    // Original Debug reference: VCPolyhedron::Init(VCVertex *, int, VCEdge *, int, VCFace *, int, VCVoronoiPlane *, int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Init();
    // ELF 0x001D3398; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d3398.md.
    // Original Debug reference: VCPolyhedron::NewVoronoiPlanes(int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void NewVoronoiPlanes();
    // ELF 0x001D3430; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d3430.md.
    // Original Debug reference: VCPolyhedron::NewVertex(bVector3 *, int, VCVoronoiPlane *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void NewVertex();
    // ELF 0x001D3498; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d3498.md.
    // Original Debug reference: VCPolyhedron::NewEdge(bVector3 *, int, VCVoronoiPlane *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void NewEdge();
    // ELF 0x001D3540; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d3540.md.
    // Original Debug reference: VCPolyhedron::NewFace(bVector3 *, float, int, VCVoronoiPlane *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void NewFace();
    // ELF 0x001D3BA8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d3ba8.md.
    // Original Debug reference: VCPolyhedron::GetFeatureCode(VCFeature *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetFeatureCode();
    // ELF 0x001D3C60; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d3c60.md.
    // Original Debug reference: VCPolyhedron::GetFeature(unsigned int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetFeature();
};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class VCTransform {
public:
    // TASK-0048 E-0048-T05; Debug original feature-walk host binding.
    static port::track::VoronoiTransform Sub_001CCF28(const port::track::VoronoiPose&,const port::track::VoronoiPose&);
    // ELF 0x001D0058; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d0058.md.
    // Original Debug reference: VCTransform::TransformVector(fVector3); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void TransformVector();
    // ELF 0x001D00B0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d00b0.md.
    // Original Debug reference: VCTransform::TransformPoint(fVector3); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void TransformPoint();
};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class VCTransformedFeature {
public:
    // TASK-0048 E-0048-T05; Debug original feature-walk host binding.
    static port::track::VoronoiTransformedFeature TransformVertex(const port::track::VoronoiPolyhedron&,std::uint32_t,const port::track::VoronoiTransform&);
    static port::track::VoronoiTransformedFeature TransformEdge(const port::track::VoronoiPolyhedron&,std::uint32_t,const port::track::VoronoiTransform&);
    // ELF 0x001D00F8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d00f8.md.
    // Original Debug reference: VCTransformedFeature::TransformVertex(VCTransform *, VCVertex *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void TransformVertex();
    // ELF 0x001D0168; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d0168.md.
    // Original Debug reference: VCTransformedFeature::TransformEdge(VCTransform *, VCEdge *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void TransformEdge();
};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class VCVertex {
public:
    // Debug 0x001CF6A8; E-0048-T03; host feature storage binding.
    static void SetVoronoiPlanes(port::track::VoronoiPolyhedron&,unsigned);
    // ELF 0x001D2800; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d2800.md.
    // Original Debug reference: VCVertex::SetVoronoiPlanes(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void SetVoronoiPlanes();
    // ELF 0x001D2950; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d2950.md.
    // Original Debug reference: VCVertex::GetNextEdgeInFace(VCEdge *, VCFace *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetNextEdgeInFace();
    // ELF 0x001D29E8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d29e8.md.
    // Original Debug reference: VCVertex::Move(bVector3 *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Move();
};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class VCVoronoiClip {
public:
    // TASK-0048 E-0048-T05; Debug original feature-walk host binding.
    static bool ClipPlanes(port::track::VoronoiClip&,const port::track::VoronoiTransformedFeature&,const std::vector<port::track::VoronoiPlaneNeighbor>&);
    static std::uint32_t ClipVertexPlanes(port::track::VoronoiClip&,const port::track::VoronoiPolyhedron&,const port::track::VoronoiTransformedFeature&,const std::array<float,3>*,const std::vector<port::track::VoronoiPlaneNeighbor>&);
    static std::uint32_t ClipFacePlanes(port::track::VoronoiClip&,const port::track::VoronoiPolyhedron&,const port::track::VoronoiTransformedFeature&,const port::track::VoronoiFace*,const std::vector<port::track::VoronoiPlaneNeighbor>&);
    // ELF 0x001D0228; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d0228.md.
    // Original Debug reference: VCVoronoiClip::VCVoronoiClip(void); constructor/destructor entry is not a native C++ lifetime operation.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001D0228();
    // ELF 0x001D0250; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d0250.md.
    // Original Debug reference: VCVoronoiClip::ClipPlanes(VCTransformedFeature *, VCVoronoiPlane *, int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void ClipPlanes();
};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class VCWitness {
public:
    // Debug 0x001CE5F8; E-0048-T05.
    static int SubCheckEdgeEdge(port::track::VoronoiWitness&,const port::track::VoronoiPolyhedron&,std::uint32_t,const port::track::VoronoiTransformedFeature&,int);
    // Debug 0x001CDA08; E-0048-T05.
    static int CheckVertexVertex(port::track::VoronoiWitness&,const port::track::VoronoiPolyhedron&,const port::track::VoronoiPolyhedron&,const port::track::VoronoiTransform&,const port::track::VoronoiTransform&);
    // Debug 0x001CDD00; E-0048-T05.
    static int CheckVertexEdge(port::track::VoronoiWitness&,const port::track::VoronoiPolyhedron&,const port::track::VoronoiPolyhedron&,const port::track::VoronoiTransform&,const port::track::VoronoiTransform&);
    // Debug 0x001CE038; E-0048-T05.
    static int CheckVertexFace(port::track::VoronoiWitness&,const port::track::VoronoiPolyhedron&,const port::track::VoronoiPolyhedron&,const port::track::VoronoiTransform&,const port::track::VoronoiTransform&);
    // Debug 0x001CE6D8; E-0048-T05.
    static int CheckEdgeEdge(port::track::VoronoiWitness&,const port::track::VoronoiPolyhedron&,const port::track::VoronoiPolyhedron&,const port::track::VoronoiTransform&,const port::track::VoronoiTransform&);
    // Debug 0x001CEC10; E-0048-T05.
    static int CheckEdgeFace(port::track::VoronoiWitness&,const port::track::VoronoiPolyhedron&,const port::track::VoronoiPolyhedron&,const port::track::VoronoiTransform&,const port::track::VoronoiTransform&);
    // TASK-0048 E-0048-T05; Debug original feature-walk host binding.
    static int VClip(port::track::VoronoiWitness&,const port::track::VoronoiPolyhedron&,const port::track::VoronoiPose&,const port::track::VoronoiPolyhedron&,const port::track::VoronoiPose&);
    // ELF 0x001D0890; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001d0890.md.
    // Original Debug reference: VCWitness::VCWitness(VCPolyhedron *, VCPolyhedron *); constructor/destructor entry is not a native C++ lifetime operation.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001D0890();
};
} // namespace hp2::recovered
