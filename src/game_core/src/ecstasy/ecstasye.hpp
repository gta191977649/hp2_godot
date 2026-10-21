// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include <set>
#include "game_core/port/driving/skid_services.hpp"
#include "game_core/port/track/effects.hpp"
#include "game_core/port/track/runtime.hpp"
#include "game_core/port/track/postprocess.hpp"

namespace hp2::recovered {
// Retail00104448 / Debug001042E8; original global ScreenWidth and bAngle math.
float CalculateH(port::driving::Memory&,std::uint16_t horizontal_fov);
// TASK0052 E-0052-P03, static viewport quad slices; original TU ecstasye.cpp.
std::optional<port::track::postprocess::Quad> DisplayFilter(const port::track::postprocess::View&,const port::track::postprocess::ScreenControls&);
std::vector<port::track::postprocess::Quad> DisplayRadialBlur(const port::track::postprocess::View&,const port::track::postprocess::ScreenControls&);
std::optional<port::track::postprocess::Quad> DisplayBlendPrevious(const port::track::postprocess::View&,const port::track::postprocess::ScreenControls&);
// TASK0051 bounded track command stage of original eDisplayFrame, excluding
// vehicles/frontend/screen effects and hardware DMA ownership.
port::track::Frame eDisplayFrame(const port::track::Scene&,const port::track::FrameView&,
    std::vector<std::size_t>&,port::driving::Memory&,std::vector<std::uint32_t>* texture_chain_order=nullptr,const port::track::ResourceIndex* resource_index=nullptr);
}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class eDmaChain {
public:
    // ELF 0x001047E0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001047e0.md.
    // Original Debug reference: eDmaChain::Init(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Init();
};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class eModel {
public:
    // Retail0010DC80 / Debug0010E048, supported static nonanimated branch.
    static void Render(const port::track::Scene&,const port::track::FrameView&,
        const port::track::DrawCommand& model_draw,port::track::Frame&);
    // Retail0010CB00 / Debug0010CEF8, explicit eSolid::StripFlags carrier.
    static bool IsLit(std::uint16_t solid_strip_flags);
    // TASK0051 bounded static strip selection slice of Retail0010DD58.
    // Caller flags and visibility are original values, not Godot flags. DMA,
    // animation/replacement tables and allocation remain outside this slice.
    static std::optional<port::track::EffectState> RenderStrips(
        const hp2::assets::TpkTextureHeader&,std::uint16_t strip_flags,
        std::uint32_t caller_flags,bool has_light_context,bool enable_environment,
        bool target_suppresses_alpha,bool partially_visible,bool depth_route_enabled=true);
    // ELF 0x0010C5B0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0010c5b0.md.
    // Original Debug reference: eModel::UnInit(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void UnInit();
    // ELF 0x0010C878; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0010c878.md.
    // Original Debug reference: eModel::ApplyReplacementTextureTable(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void ApplyReplacementTextureTable();
    // ELF 0x0010CB00; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0010cb00.md.
    // Original Debug reference: eModel::IsLit(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void IsLit();
};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class ePoly {
public:
    // Debug0010FD48/00110368; E-0046-S02. Host projection of original fields,
    // not a PS2 layout. Render3D stops at explicit DMA submission boundary.
    std::array<port::driving::SkidVector,4> Vertices{};
    std::array<std::array<std::uint8_t,4>,4> Colours{};
    std::array<std::array<float,3>,4> UVs{};
    std::uint8_t AlphaA=0,AlphaB=0,AlphaC=0,AlphaD=0,AlphaFix=0;
    bool AlphaEnabled=false,ApplyAspectRatio=false,SunPoly=false,Flailer=false;
    void Sub_0010FD48();
    void Render3D(const port::driving::SkidTexture&,int terrain,port::driving::SkidServices&);


};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class eSolid {
public:
    // Retail0010BDB8 / Debug0010C1B0; E0056T01. Original table-name iteration
    // and GetTextureInfo(name,1,0), with explicit owning host resource sets.
    static std::vector<std::uint32_t> FixTextureTable(const std::vector<std::uint32_t>&,
        const std::set<std::uint32_t>& loaded,const std::set<std::uint32_t>& render_targets);
    // ELF 0x00103FF0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/vehicle/00103ff0.md.
    // Original Debug reference: eSolid::FixStripEntryTable(unsigned char *, int, unsigned char *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void FixStripEntryTable();
    // ELF 0x00104070; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00104070.md.
    // Original Debug reference: eSolid::SetEnvironmentMapping(unsigned int, int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void SetEnvironmentMapping();
    // ELF 0x0010BD38; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0010bd38.md.
    // Original Debug reference: eSolid::GetBoundingBox(bVector3 *, bVector3 *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetBoundingBox();
    // ELF 0x0010C158; E-0042-C002; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/vehicle/0010c158.md.
    // Original Debug reference: eSolid::FixStripSTs(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void FixStripSTs();
    // ELF 0x0010C4B0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0010c4b0.md.
    // Original Debug reference: eSolid::ReplaceLightMaterial(unsigned int, eLightMaterial *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void ReplaceLightMaterial();
    // ELF 0x00114BB0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00114bb0.md.
    // Original Debug reference: eSolid::eSolid(eSolid const &); constructor/destructor entry is not a native C++ lifetime operation.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_00114BB0();
};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class eTextureDownloader {
public:
    // ELF 0x00109260; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00109260.md.
    // Original Debug reference: eTextureDownloader::Init(int, int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Init();
};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class eTransferRectQueue {
public:
    // ELF 0x00110C60; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00110c60.md.
    // Original Debug reference: eTransferRectQueue::eTransferRectQueue(void); constructor/destructor entry is not a native C++ lifetime operation.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_00110C60();
    // ELF 0x00110CD8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00110cd8.md.
    // Original Debug reference: eTransferRectQueue::GetTransferRect(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetTransferRect();
};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class eView {
public:
    // Debug00107410; E-0046-S02. Explicit original H field argument, host binding.
    static int GetPixelSize(float radius,float distance,float H);

    // ELF 0x001075B8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001075b8.md.
    // Original Debug reference: eView::GetPixelSize(float, float); multiple original overloads cannot share a void/no-argument placeholder.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001075B8();
    // ELF 0x00107600; E-0042-C001; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/vehicle/00107600.md.
    // Original Debug reference: eView::GetPixelSize(bVector3 const *, float); multiple original overloads cannot share a void/no-argument placeholder.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_00107600();
    // ELF 0x001076F8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001076f8.md.
    // Original Debug reference: eView::GetPixelSize(bVector3 const *, bVector3 const *); multiple original overloads cannot share a void/no-argument placeholder.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001076F8();
};
} // namespace hp2::recovered

namespace hp2::recovered {
// Debug0010D0A8/0010D278; E-0046-S02, original TU src/ecstasy/ecstasye.cpp.
std::uint32_t ClassifyAABB(const port::driving::SkidVector&,const port::driving::SkidVector&,const std::array<float,16>&);
bool IsBoundingBoxVisible(const port::driving::SkidVector&,const port::driving::SkidVector&,const std::array<float,16>&);
}

namespace hp2::recovered {
// Debug0010BEF0/0010BBF8; E-0046-S04. Finite texture-to-normal-target selection
// slice only; explicit host fields replace TextureInfo/target and eGetZWriteState.
port::driving::SkidDrawState AttachSolidPolys(const port::driving::SkidTexture&,bool depth_route_enabled);
port::driving::SkidDrawState AttachAlphaPolys(const port::driving::SkidTexture&,bool depth_route_enabled);
}
