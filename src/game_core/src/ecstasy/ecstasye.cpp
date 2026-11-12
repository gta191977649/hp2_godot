// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/src/ecstasy/ecstasye.hpp"

#include "game_core/port/not_implemented.hpp"
#include "game_core/indep/inc/Scenery.hpp"
#include "game_core/indep/src/texture.hpp"

#include "game_core/src/bMath.hpp"
namespace hp2::recovered {
// Retail0010BDB8 / Debug0010C1B0; E0056T01, reverse/task0056/textures.md.
std::vector<std::uint32_t> eSolid::FixTextureTable(const std::vector<std::uint32_t>& names,
    const std::set<std::uint32_t>& loaded,const std::set<std::uint32_t>& targets) {
    std::vector<std::uint32_t> result;result.reserve(names.size());
    for(auto name:names)result.push_back(*GetTextureInfo(name,true,loaded,targets));
    return result;
}
// Retail00104448 / Debug001042E8. Debug memory binding supplies original globals;
// the independently inspected Retail ScreenWidth is also512 in both video modes.
float CalculateH(port::driving::Memory& math,std::uint16_t horizontal_fov){
    const auto half=std::uint16_t(horizontal_fov>>1);
    const float sine=hp2::recovered::bSin(math,half);
    const float cosine=hp2::recovered::bSin(math,std::uint16_t(half+0x4000)); // original bCos tailcall
    const float tangent=sine/cosine;
    if(tangent==0.0f)return 10.0f;
    return float(math.signed_word(0x00326760)>>1)/tangent;
}

// ELF 0x001047E0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001047e0.md.
void eDmaChain::Init() {
    hp2::not_implemented("0x001047E0", "eDmaChain::Init (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x0010C5B0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0010c5b0.md.
void eModel::UnInit() {
    hp2::not_implemented("0x0010C5B0", "eModel::UnInit (signature and full body unresolved)");
}

// ELF 0x0010C878; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0010c878.md.
void eModel::ApplyReplacementTextureTable() {
    hp2::not_implemented("0x0010C878", "eModel::ApplyReplacementTextureTable (signature and full body unresolved)");
}

// ELF 0x0010CB00; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0010cb00.md.
void eModel::IsLit() {
    hp2::not_implemented("0x0010CB00", "eModel::IsLit (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {

} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x00103FF0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/vehicle/00103ff0.md.
void eSolid::FixStripEntryTable() {
    hp2::not_implemented("0x00103FF0", "eSolid::FixStripEntryTable (signature and full body unresolved)");
}

// ELF 0x00104070; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00104070.md.
void eSolid::SetEnvironmentMapping() {
    hp2::not_implemented("0x00104070", "eSolid::SetEnvironmentMapping (signature and full body unresolved)");
}

// ELF 0x0010BD38; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0010bd38.md.
void eSolid::GetBoundingBox() {
    hp2::not_implemented("0x0010BD38", "eSolid::GetBoundingBox (signature and full body unresolved)");
}

// ELF 0x0010C158; E-0042-C002; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/vehicle/0010c158.md.
void eSolid::FixStripSTs() {
    hp2::not_implemented("0x0010C158", "eSolid::FixStripSTs (signature and full body unresolved)");
}

// ELF 0x0010C4B0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0010c4b0.md.
void eSolid::ReplaceLightMaterial() {
    hp2::not_implemented("0x0010C4B0", "eSolid::ReplaceLightMaterial (signature and full body unresolved)");
}

// ELF 0x00114BB0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00114bb0.md.
void eSolid::Sub_00114BB0() {
    hp2::not_implemented("0x00114BB0", "eSolid::Sub_00114BB0 (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x00109260; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00109260.md.
void eTextureDownloader::Init() {
    hp2::not_implemented("0x00109260", "eTextureDownloader::Init (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x00110C60; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00110c60.md.
void eTransferRectQueue::Sub_00110C60() {
    hp2::not_implemented("0x00110C60", "eTransferRectQueue::Sub_00110C60 (signature and full body unresolved)");
}

// ELF 0x00110CD8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00110cd8.md.
void eTransferRectQueue::GetTransferRect() {
    hp2::not_implemented("0x00110CD8", "eTransferRectQueue::GetTransferRect (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x001075B8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001075b8.md.
void eView::Sub_001075B8() {
    hp2::not_implemented("0x001075B8", "eView::Sub_001075B8 (signature and full body unresolved)");
}

// ELF 0x00107600; E-0042-C001; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/vehicle/00107600.md.
void eView::Sub_00107600() {
    hp2::not_implemented("0x00107600", "eView::Sub_00107600 (signature and full body unresolved)");
}

// ELF 0x001076F8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001076f8.md.
void eView::Sub_001076F8() {
    hp2::not_implemented("0x001076F8", "eView::Sub_001076F8 (signature and full body unresolved)");
}
} // namespace hp2::recovered

// TASK-0046 E-0046-S02. Debug815b60c7...332f1b, original TU src/ecstasy/ecstasye.cpp.
#include <cmath>
#include <stdexcept>
namespace hp2::recovered {
// Debug0010FD48, original ePoly constructor. Explicit host initialization.
void ePoly::Sub_0010FD48(){
    Vertices={{{{0,0,0}},{{64,0,0}},{{64,64,0}},{{0,64,0}}}};
    for(auto& c:Colours)c={{0x80,0x80,0x80,0x80}};
    UVs={{{{0,0,1}},{{1,0,1}},{{1,1,1}},{{0,1,1}}}};
    AlphaA=0;AlphaB=1;AlphaC=0;AlphaD=1;AlphaFix=0x80;
    AlphaEnabled=false;ApplyAspectRatio=true;SunPoly=false;Flailer=false;
}
// Debug00110368. Supported finite identity-world-matrix Render3D submission slice.
// Original DMA receives vertices in strip order0,1,3,2. Host commands retain quad
// order0,1,2,3; render backend must submit the equivalent strip/triangles.
void ePoly::Render3D(const port::driving::SkidTexture& texture,int terrain,port::driving::SkidServices&services){
    if(!texture.present)return;
    if(!services.emit)hp2::not_implemented("0x00110368","ePoly::Render3D host DMA submission service");
    port::driving::SkidQuad q;q.vertices=Vertices;q.colours=Colours;
    for(unsigned i=0;i<4;++i)q.uv[i]={{UVs[i][0],UVs[i][1]}};
    q.terrain=terrain;q.texture_name=texture.name;q.texture_hash=texture.hash;
    q.semitransparent=texture.semitransparent;q.alpha_sort=texture.alpha_sort;
    const auto draw=texture.alpha_sort?AttachAlphaPolys(texture,services.depth_route_enabled):AttachSolidPolys(texture,services.depth_route_enabled);
    q.draw_context=draw.draw_context;q.alpha_test=draw.alpha_test;q.alpha_reference=draw.alpha_reference;q.write_depth=draw.write_depth;
    services.emit(q);
}
// Debug00107410, exact original conversion/truncation and near-radius branch.
int eView::GetPixelSize(float radius,float distance,float H){
    float result=H;
    if(radius*0.5f<distance)result=(radius*result)/distance;
    else result=result+result;
    if(!std::isfinite(result)||result>=2147483648.0f||result< -2147483648.0f)
        hp2::not_implemented("0x00107410","nonfinite or out-of-range EE integer conversion");
    return static_cast<int>(result);
}
// Debug0010D0A8: VU ClassifyAABB, finite ordinary-float projection. The eight
// corner clip flags are ANDed for trivial rejection, ORed for full inclusion.
std::uint32_t ClassifyAABB(const port::driving::SkidVector& lo,const port::driving::SkidVector& hi,const std::array<float,16>&m){
    std::array<float,4> base{},dx{},dy{},dz{};
    for(unsigned i=0;i<4;++i){float acc=m[i]*lo[0];acc=acc+m[4+i]*lo[1];acc=acc+m[8+i]*lo[2];base[i]=acc+m[12+i];
        dx[i]=m[i]*(hi[0]-lo[0]);dy[i]=m[4+i]*(hi[1]-lo[1]);dz[i]=m[8+i]*(hi[2]-lo[2]);}
    unsigned all=0x3f,any=0;
    for(unsigned z=0;z<2;++z)for(unsigned j=0;j<4;++j){
        std::array<float,4> p=base;
        if(j==1||j==2)for(unsigned i=0;i<4;++i)p[i]=p[i]+dx[i];
        if(j==2||j==3)for(unsigned i=0;i<4;++i)p[i]=p[i]+dy[i];
        if(z)for(unsigned i=0;i<4;++i)p[i]=p[i]+dz[i];
        unsigned mask=0;const float w=std::abs(p[3]);
        for(unsigned i=0;i<3;++i){if(p[i]>w)mask|=1u<<(i*2);if(p[i]< -w)mask|=2u<<(i*2);}
        all&=mask;any|=mask;
    }
    if(!any)return 2;
    return all?0:1;
}
// Debug0010D278.
bool IsBoundingBoxVisible(const port::driving::SkidVector&lo,const port::driving::SkidVector&hi,const std::array<float,16>&m){return ClassifyAABB(lo,hi,m)!=0;}
}

namespace hp2::recovered {
// Debug0010BEF0; E-0046-S04. Selection slice independently checked against
// 0010BFD0..0010C00C. Debug uses RenderingOrder29, not TextureInfo.bWriteZ50.
port::driving::SkidDrawState AttachSolidPolys(const port::driving::SkidTexture&t,bool enabled){
    unsigned context;
    if(t.gloss_map_in_alpha)context=3;
    else if(!t.semitransparent)context=2;
    else context=t.rendering_order<3?3:1;
    context&=enabled?0xffffu:0xfffdu;
    return {context,(context&1)?5u:6u,(context&1)?0u:48u,(context&2)!=0};
}
// Debug0010BBF8; E-0046-S04. Alpha-list route omits the solid GlossMap branch.
port::driving::SkidDrawState AttachAlphaPolys(const port::driving::SkidTexture&t,bool enabled){
    unsigned context;
    if(!t.semitransparent)context=2;
    else context=t.rendering_order<3?3:1;
    context&=enabled?0xffffu:0xfffdu;
    return {context,(context&1)?5u:6u,(context&1)?0u:48u,(context&2)!=0};
}
}
// TASK0051: static command projection, Retail0010DD58. See reverse/task0051/blend.md.
bool hp2::recovered::eModel::IsLit(std::uint16_t solid_strip_flags) {
    return (solid_strip_flags&0x1c0)!=0;
}
std::optional<port::track::EffectState> hp2::recovered::eModel::RenderStrips(
    const hp2::assets::TpkTextureHeader& texture,std::uint16_t strip_flags,
    std::uint32_t caller_flags,bool has_light_context,bool enable_environment,
    bool target_suppresses_alpha,bool partially_visible,bool depth_route_enabled) {
    const bool alpha=texture.Field_0x4d!=0;
    if((strip_flags&0x400)!=0||((target_suppresses_alpha||(caller_flags&0x10)!=0)&&alpha)||
       ((caller_flags&8)!=0&&!alpha))return std::nullopt;
    std::uint16_t clear=0;
    if(!enable_environment||(caller_flags&1)!=0)clear|=0x100;
    if((caller_flags&2)!=0)clear|=0x80;
    if(!has_light_context)clear=0x1c0;
    if(texture.Field_0x74!=0)strip_flags|=4;
    if(partially_visible)strip_flags|=0x200;
    return port::track::effect_state(texture,std::uint16_t(strip_flags&~clear),depth_route_enabled);
}

void hp2::recovered::eModel::Render(const port::track::Scene& scene,
    const port::track::FrameView& view,const port::track::DrawCommand& model_draw,port::track::Frame& frame) {
    // Retail0010DC80 static nonanimated branch preserves supplied local-world
    // and light context, then dispatches the solid strip list via RenderStrips.
    const auto& solid=scene.geometry.solids.at(model_draw.solid);
    for(std::size_t surface_index=0;surface_index<solid.surfaces.size();++surface_index){
        auto command=model_draw;command.surface=surface_index;
        command.original_order=frame.submitted_surfaces++;
        const auto& surface=solid.surfaces[surface_index];
        const auto* texture=frame.resource_index?&frame.resource_index->texture(scene,surface.texture_hash):nullptr;
        const auto linear=texture?scene.textures.end():std::find_if(scene.textures.begin(),scene.textures.end(),[&](const auto& t){return t.header.tex_hash==surface.texture_hash;});
        if(!texture){if(linear==scene.textures.end())throw std::runtime_error("Track model has unresolved texture");texture=&*linear;}
        auto state=eModel::RenderStrips(texture->header,surface.render_flags,
            command.visibility==2?4:0,command.has_light_context,true,false,command.visibility==1);
        if(!state)continue;
        command.effect=*state;
        // Original0010CBF8 transforms midpoint of strip-entry AABB(+10,+20),
        // first by local-world, then world-view; retain native camera-forward z.
        std::array<float,3> center{},world{};
        for(unsigned i=0;i<3;++i){float lo,hi;std::memcpy(&lo,surface.strip_entry.data()+0x10+i*4,4);std::memcpy(&hi,surface.strip_entry.data()+0x20+i*4,4);center[i]=(lo+hi)*.5f;}
        for(unsigned i=0;i<3;++i)world[i]=command.matrix[12+i]+center[0]*command.matrix[i]+center[1]*command.matrix[4+i]+center[2]*command.matrix[8+i];
        command.depth=0;for(unsigned i=0;i<3;++i)command.depth+=(world[i]-view.position[i])*view.direction[i];
        frame.commands.push_back(command);
    }
}

port::track::Frame hp2::recovered::eDisplayFrame(const port::track::Scene& scene,
    const port::track::FrameView& view,std::vector<std::size_t>& zones,port::driving::Memory& math,
    std::vector<std::uint32_t>* texture_chain_order,const port::track::ResourceIndex* resource_index) {
    port::track::Frame frame;frame.resource_index=resource_index;
    GrandSceneryCullInfo::DoCulling(scene,view,zones,math,frame);
    // Sort references to immutable frame commands, not their large matrix/light payloads.
    std::vector<std::size_t> opaque,alpha;
    std::vector<port::track::AlphaKey> keys;
    opaque.reserve(frame.commands.size());alpha.reserve(frame.commands.size());keys.reserve(frame.commands.size());
    std::vector<std::uint32_t> temporary_chains;
    auto& chains=texture_chain_order?*texture_chain_order:temporary_chains;
    std::unordered_map<std::uint32_t,bool> seen_chains;seen_chains.reserve(chains.size()+64);for(auto hash:chains)seen_chains.emplace(hash,true);
    for(std::size_t index=0;index<frame.commands.size();++index){
        const auto& command=frame.commands[index];
        const auto& surface=scene.geometry.solids.at(command.solid).surfaces.at(command.surface);
        if(seen_chains.emplace(surface.texture_hash,true).second)chains.push_back(surface.texture_hash);
        if(command.effect.alpha_pass){keys.push_back({command.effect.rendering_order,command.depth});alpha.push_back(index);}
        else opaque.push_back(index);
    }
    // Retail00109358: non-deferred texture chains are bucketed by +29 too.
    // First-use texture chains and shared image-parent sibling grouping follow
    // original static-resident order; paging uploads remain a host boundary.
    const auto hash=[&](const auto& c){return scene.geometry.solids[c.solid].surfaces[c.surface].texture_hash;};
    std::vector<std::uint32_t> grouped;
    std::map<std::pair<int,std::uint32_t>,std::uint32_t> first_parent;
    // Preserve first-use and image-parent sibling reversal; index lookups only
    // replace repeated linear searches, never the original logical ordering.
    std::unordered_map<std::uint32_t,const hp2::assets::TpkTextureHeader*> headers;
    headers.reserve(scene.textures.size());for(const auto& t:scene.textures)headers.emplace(t.header.tex_hash,&t.header);
    for(int bucket=0;bucket<8;++bucket)for(auto chain:chains){
        const auto found=headers.find(chain);if(found==headers.end())continue;const auto& t=*found->second;
        if(t.RenderingOrder!=bucket||t.Field_0x4d!=0)continue;
        const auto parent=t.ImageParentHash?t.ImageParentHash:chain;
        const auto key=std::make_pair(bucket,parent);const auto prior=first_parent.find(key);
        if(prior==first_parent.end()){first_parent.emplace(key,chain);grouped.push_back(chain);}
        else grouped.insert(std::find(grouped.begin(),grouped.end(),prior->second)+1,chain);
    }
    std::unordered_map<std::uint32_t,std::size_t> ranks;ranks.reserve(grouped.size());for(std::size_t i=0;i<grouped.size();++i)ranks.emplace(grouped[i],i);
    std::stable_sort(opaque.begin(),opaque.end(),[&](auto ai,auto bi){
        const auto& a=frame.commands[ai];const auto& b=frame.commands[bi];
        if(a.effect.rendering_order!=b.effect.rendering_order)return a.effect.rendering_order<b.effect.rendering_order;
        const auto aa=ranks.find(hash(a)),bb=ranks.find(hash(b));
        return (aa==ranks.end()?grouped.size():aa->second)<(bb==ranks.end()?grouped.size():bb->second);
    });
    for(auto index:port::track::alpha_order_001055B0(keys))opaque.push_back(alpha[index]);
    std::vector<port::track::DrawCommand> ordered;ordered.reserve(opaque.size());
    for(auto index:opaque)ordered.push_back(std::move(frame.commands[index]));
    frame.commands=std::move(ordered);frame.resource_index=nullptr;return frame;
}
// TASK0052 E-0052-P03. Retail0010F6A8 / Debug0010FA88.
std::optional<port::track::postprocess::Quad> hp2::recovered::DisplayFilter(
    const port::track::postprocess::View& v,const port::track::postprocess::ScreenControls& s){
    using namespace port::track::postprocess;
    if((v.id!=1&&v.id!=2)||!v.target_active||!s.filter.texture_available||!s.filter.bActive)return std::nullopt;
    Quad q;q.vertices={{{{float(v.scissor_x),float(v.scissor_y)}},{{float(v.scissor_x+v.scissor_w),float(v.scissor_y)}},
        {{float(v.scissor_x+v.scissor_w),float(v.scissor_y+v.scissor_h)}},{{float(v.scissor_x),float(v.scissor_y+v.scissor_h)}}}};
    for(auto& colour:q.colours)colour=s.filter.aColour;
    q.blend=s.filter.alpha;q.texture_hash=0x02c8711c;q.texture_fx=s.filter.nTextureFx;return q;
}
// Retail0010F280 / Debug0010F660. Four mirrored quarters write alpha.
std::vector<port::track::postprocess::Quad> hp2::recovered::DisplayRadialBlur(
    const port::track::postprocess::View& v,const port::track::postprocess::ScreenControls& s){
    using namespace port::track::postprocess;std::vector<Quad> result;
    if((v.id!=1&&v.id!=2)||!v.target_active||!s.blend_mask_available)return result;
    const float left=float(v.scissor_x),right=float(v.scissor_x+v.scissor_w),top=float(v.scissor_y),bottom=float(v.scissor_y+v.scissor_h);
    const float mx=float(v.scissor_x+v.scissor_w/2),my=float(v.scissor_y+v.scissor_h/2);
    for(unsigned i=0;i<4;++i){Quad q;const float x=i==1||i==2?right:left,y=i<2?top:bottom;
        q.vertices={{{{x,y}},{{mx,y}},{{mx,my}},{{x,my}}}};
        for(unsigned j=0;j<4;++j)q.colours[j]={{128,128,128,j==2?s.nRadialAlphaCenter:s.nRadialAlphaEdge}};
        q.blend={0,0,0,1,128};q.texture_hash=0xab9a6e21;result.push_back(q);}
    return result;
}
// Retail0010F4F0 / Debug0010F8D0. Previous MainDisp0 uses512-square backing UVs.
std::optional<port::track::postprocess::Quad> hp2::recovered::DisplayBlendPrevious(
    const port::track::postprocess::View& v,const port::track::postprocess::ScreenControls& s){
    using namespace port::track::postprocess;
    if(!v.active||!v.target_active||!s.blend_mask_available||!s.previous_frame_available)return std::nullopt;
    Quad q;q.vertices={{{{float(v.scissor_x),float(v.scissor_y)}},{{float(v.scissor_x+v.scissor_w),float(v.scissor_y)}},
        {{float(v.scissor_x+v.scissor_w),float(v.scissor_y+v.scissor_h)}},{{float(v.scissor_x),float(v.scissor_y+v.scissor_h)}}}};
    const float u0=(v.scissor_x+.5f)/512.f,v0=(v.scissor_y+.5f)/512.f,u1=(v.scissor_w-.5f)/512.f,v1=(v.scissor_h-.5f)/512.f;
    q.uv={{{{u0,v0}},{{u1,v0}},{{u1,v1}},{{u0,v1}}}};for(auto& c:q.colours)c={{128,128,128,128}};
    q.blend={0,1,1,1,128};q.previous_frame=true;return q;
}

#include "game_core/port/race/car_render_resources.hpp"
#include "game_core/port/race/debug_transport.hpp"
#include "game_core/port/track/texture_bindings.hpp"
#include "game_core/inc/bSlotPool.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include "game_core/port/driving/projections.hpp"
#include <cstring>
namespace port::race {
// Debug0010C918; E-0099-W25. First match preserves original duplicate order.
std::uint32_t eFindSolid(Memory&m,std::uint32_t hash){for(auto p=m.word(0x326718);p!=0x326718;p=m.word(p))if(m.word(p+8)==hash)return p;return 0;}
// Debug0010C950; E-0099-W25. Caller constructs NameHash/Solid before Init.
void eModel::Init(std::uint32_t name){
 m_.word(address_+8,name);m_.word(address_+16,0);m_.byte(address_+20,0);m_.half(address_+22,0);
 if(name){ReconnectSolid();const auto tail=m_.word(0x326724);m_.word(tail,address_);m_.word(0x326724,address_);m_.word(address_,0x326720);m_.word(address_+4,tail);}
}
// Debug0010C9A8; E-0099-W25. Zero-name branch does not overwrite other bytes.
void eModel::UnInit(){if(m_.word(address_+8)){
 const auto prev=m_.word(address_+4),next=m_.word(address_);m_.byte(address_+20,0);m_.word(prev,next);m_.word(next+4,prev);
 m_.word(address_+12,0);m_.word(address_+8,0);m_.word(address_+16,0);m_.half(address_+22,0);
}}
// Debug0010C9E0; E-0099-W25. Original rigid start state and optional RNG draw.
void eModel::ReconnectSolid(){if(const auto name=m_.word(address_+8)){
 const auto solid=eFindSolid(m_,name);if(solid!=m_.word(address_+12)){
  m_.word(address_+12,solid);m_.half(address_+22,0);
  if(solid)if(const auto animation=m_.word(solid+0xa8))if(m_.byte(animation+2)==0){
   m_.half(address_+22,0xc000);if(m_.half(solid+0x30)&0x20)m_.half(address_+22,static_cast<std::uint16_t>(0xc000|hp2::recovered::bRandom(m_,256)));
  }
 }
}}
// Debug0010C1B0; E-0099-W25. Real shared-Memory texture provider mutates LRU.
void FixTextureTable(Memory&m,std::uint32_t solid){for(int i=0;i<m.signed_half(solid+0xac);++i){const auto p=m.word(solid+0xb8)+8*static_cast<unsigned>(i);m.word(p+4,hp2::debug::GetTextureInfo(m,m.word(p),1,0));}}
// Debug0010C850; E-0099-W25.
void FixLightMaterialTable(Memory&m,std::uint32_t solid){for(int i=0;i<m.signed_half(solid+0xae);++i){const auto p=m.word(solid+0xbc)+8*static_cast<unsigned>(i);m.word(p+4,elGetLightMaterial(m,m.word(p)));}}
// Debug0010CC70; E-0099-W25.
void eModel::ApplyReplacementTextureTable(){if(const auto solid=m_.word(address_+12))for(int i=0;i<m_.signed_byte(address_+20);++i){
 const auto entry=m_.word(address_+16)+28*static_cast<unsigned>(i);const int index=m_.signed_byte(entry+12+static_cast<unsigned>(m_.signed_byte(address_+21)));
 if(index>=0)m_.word(m_.word(solid+0xb8)+8*static_cast<unsigned>(index)+4,m_.word(entry+8));
}}
// Debug0010CA78; E-0099-W25. Per-instance signed indices and original table scan.
void eModel::ReconnectTextureTable(){for(int i=0;i<m_.signed_byte(address_+20);++i){
 const auto entry=m_.word(address_+16)+28*static_cast<unsigned>(i),index=entry+12+static_cast<unsigned>(m_.signed_byte(address_+21));
 m_.word(entry+8,hp2::debug::GetTextureInfo(m_,m_.word(entry+4),1,0));m_.byte(index,255);
 if(const auto solid=m_.word(address_+12))for(int j=0;j<m_.signed_half(solid+0xac);++j)if(m_.word(m_.word(solid+0xb8)+8*static_cast<unsigned>(j))==m_.word(entry)){m_.byte(index,static_cast<std::uint8_t>(j));break;}
}ApplyReplacementTextureTable();}
// Debug0010CB90; E-0099-W25. Preserve double apply from original call chain.
void eModel::AttachReplacementTextureTable(std::uint32_t table,int count,int instance){
 m_.word(address_+16,table);m_.byte(address_+20,static_cast<std::uint8_t>(count));m_.byte(address_+21,static_cast<std::uint8_t>(instance));ReconnectTextureTable();
 if(const auto solid=m_.word(address_+12)){ApplyReplacementTextureTable();FixStripSTs(m_,solid);}
}
// Debug0010C550; E-0099-W25. The exponent helper uses FLOAT BITS, not numeric casts.
void FixStripSTs(Memory&m,std::uint32_t solid){
 auto scale=[](int size){float value=static_cast<float>(size);std::int32_t bits;std::memcpy(&bits,&value,4);int shift=(bits>>23)-((size&(size-1))?126:127);const auto power=std::uint32_t(1)<<(shift&31);return value/static_cast<float>(static_cast<std::int32_t>(power));};
 for(int i=0;i<m.signed_half(solid+0x34);++i){const auto entry=m.word(solid+0xb0)+64*static_cast<unsigned>(i);
  const auto texture=m.word(m.word(solid+0xb8)+8*static_cast<unsigned>(m.signed_byte(entry))+4);
  if(m.word(texture+0x20)==0x19006835)continue;
  const float sx=scale(m.signed_half(texture+0x24)),sy=scale(m.signed_half(texture+0x26));
  const bool change_x=driving::cvt_w_s(sx*32768.f)!=m.half(entry+4),change_y=driving::cvt_w_s(sy*32768.f)!=m.half(entry+6);
  if(change_x||change_y){const auto data=m.word(entry+8);const float invx=1.f/(static_cast<float>(m.half(entry+4))*0.000030517578125f),invy=1.f/(static_cast<float>(m.half(entry+6))*0.000030517578125f);
   for(int v=0;v<m.byte(data+4);++v){const auto st=GetElementST0(m,data,v);float s=m.scalar(st.first),t=m.scalar(st.second);if(change_x)s=(s*invx)*sx;if(change_y)t=(t*invy)*sy;m.scalar(st.first,s);m.scalar(st.second,t);}
   if(change_x)m.half(entry+4,static_cast<std::uint16_t>(driving::cvt_w_s(sx*32768.f)));
   if(change_y)m.half(entry+6,static_cast<std::uint16_t>(driving::cvt_w_s(sy*32768.f)));
  }
 }
}
}

namespace port::race {
// Debug00103588; E-0099-W25. Actual container child walk and pointer fixups.
// Actual inventory packet producer and platform guard are bound through E-0099-W30.
int LoaderSolidList(Memory&m,std::uint32_t chunk){
 if(m.word(chunk)!=0x80034000)return 0;
 std::string inventory="SolidList,";for(auto p=chunk+16;m.byte(p);++p)inventory+=char(m.byte(p));
 {Memory::Temporary name(m,static_cast<std::uint32_t>(inventory.size()+1));for(unsigned i=0;i<inventory.size();++i)m.byte(name.address()+i,static_cast<std::uint8_t>(inventory[i]));m.byte(name.address()+static_cast<unsigned>(inventory.size()),0);m.word(chunk+64,hp2::debug::bAddCodeineInventory(m,name.address(),0x2a3cb0));}
 const auto end=chunk+8+m.word(chunk+4);
 for(auto outer=chunk+16+m.word(chunk+12);outer<end;outer+=8+m.word(outer+4)){
  std::uint32_t solid=0;
  for(auto child=outer+8;child<outer+8+m.word(outer+4);child+=8+m.word(child+4)){
   const auto tag=m.word(child),aligned=(child+23)&~15u;
   switch(tag){
   case 0x34003:solid=aligned;break;
   case 0x34004:m.word(solid+0xb0,aligned);break;
   case 0x34006:m.word(solid+0xb8,child+8);m.half(solid+0xac,static_cast<std::uint16_t>(m.word(child+4)>>3));break;
   case 0x3401d:m.word(solid+0xbc,aligned);break;
   case 0x34005:{m.word(solid+0xb4,aligned);m.word(solid+0x3c,m.word(child+4)-(aligned-(child+8)));
    // Exact FixStripEntryTable00103E90: relative Data pointers become addresses.
    for(int i=0;i<m.signed_half(solid+0x34);++i){const auto entry=m.word(solid+0xb0)+64*static_cast<unsigned>(i);m.word(entry+8,m.word(entry+8)+aligned);}break;}
   case 0x34007:m.word(solid+0xa4,aligned);m.word(solid+0xa0,(m.word(child+4)-(aligned-(child+8)))/0x60);break;
   case 0x34012:m.word(solid+0xa8,aligned);break;
   case 0x34011:m.word(m.word(solid+0xa8)+8,child+8);break;
   default:break;
   }
  }
  if(!solid)throw std::runtime_error("Original solid container lacks header");
  FixTextureTable(m,solid);FixLightMaterialTable(m,solid);
  if(const auto external=m.word(solid+0x28))if(const auto found=eFindSolid(m,external))m.word(solid+0xa8,m.word(found+0xa8));
  const auto flags=m.half(solid+0x30);
  if(flags&0x10){
   if(const auto existing=eFindSolid(m,m.word(solid+8))){
    const auto animator=m.word(existing+0x2c);const auto count=m.signed_word(animator+12);
    if(count<64){m.word(animator+16+4*static_cast<unsigned>(count),solid);m.word(animator+12,static_cast<unsigned>(count+1));}
    m.word(solid+0x2c,animator);m.word(solid+0xa8,m.word(existing+0xa8));
   }else{
    const auto pool=m.word(0x326708);if(!pool)hp2::not_implemented("0x00109E18","Original eVertexAnimatorSlotPool startup is required");
    const auto animator=hp2::debug::bOMalloc(m,pool);m.word(animator+12,0);m.word(solid+0x2c,animator);m.word(animator+12,1);m.word(animator+8,30);m.word(animator+16,solid);
   }
  }else if(const auto existing=eFindSolid(m,m.word(solid+8))){m.half(solid+0x30,flags|2);m.half(existing+0x30,m.half(existing+0x30)|2);}
  const auto tail=m.word(0x32671c);m.word(0x3267f4,1);m.word(tail,solid);m.word(0x32671c,solid);m.word(solid,0x326718);m.word(solid+4,tail);
 }
 return 1;
}
// Debug00103950; E-0099-W25. Unlinks original solids and frees shared vertex animator once.
int UnloaderSolidList(Memory&m,std::uint32_t chunk){
 if(m.word(chunk)!=0x80034000)return 0;hp2::debug::bRemoveCodeineInventory(m,m.signed_word(chunk+64));const auto end=chunk+8+m.word(chunk+4);
 for(auto outer=chunk+16+m.word(chunk+12);outer<end;outer+=8+m.word(outer+4)){
  for(auto child=outer+8;child<outer+8+m.word(outer+4);child+=8+m.word(child+4))if(m.word(child)==0x34003){
   const auto solid=(child+23)&~15u,next=m.word(solid),prev=m.word(solid+4),animator=m.word(solid+0x2c);
   m.word(prev,next);m.word(next+4,prev);
   if(animator){const auto count=m.signed_word(animator+12);for(int i=0;i<count;++i)m.word(m.word(animator+16+4*static_cast<unsigned>(i))+0x2c,0);m.word(animator+12,0);hp2::debug::SlotPool(m,m.word(0x326708)).Free(animator);}
   break;
  }
  m.word(0x3267f4,1);
 }
 return 1;
}
// Required list members from static initializer001148A0; E-0099-W25.
void initialize_solid_model_lists(Memory&m){for(auto list:{0x326718u,0x326720u}){m.word(list+4,list);m.word(list,list);}}
}

namespace port::race {
// Debug0010CBE8; E-0099-W25. First matching authored texture-name row only.
int eModel::RenameTexture(std::uint32_t old,std::uint32_t replacement){if(const auto solid=m_.word(address_+12))for(int i=0;i<m_.signed_half(solid+0xac);++i){const auto p=m_.word(solid+0xb8)+8*static_cast<unsigned>(i);if(m_.word(p)==old){m_.word(p,replacement);m_.word(p+4,hp2::debug::GetTextureInfo(m_,replacement,1,0));return 1;}}return 0;}
// Debug0010CEB8 ->0010C8D8/0010C8A8; E-0099-W25.
void eModel::ReplaceLightMaterial(std::uint32_t old,std::uint32_t replacement){if(const auto solid=m_.word(address_+12)){const auto material=elGetLightMaterial(m_,replacement);for(int i=0;i<m_.signed_half(solid+0xae);++i){const auto p=m_.word(solid+0xbc)+8*static_cast<unsigned>(i);if(m_.word(p)==old)m_.word(p+4,material);}}}
}

namespace port::race {
// Debug00109E18; E-0099-W25. Full original allocation order and flag masks.
void eInitSlotPools(Memory&m){
 auto create=[&](unsigned global,int size,int count,unsigned name,unsigned clear){const auto pool=hp2::debug::bNewSlotPool(m,size,count,name,0x10);m.word(global,pool);if(clear)m.word(pool+0x14,m.word(pool+0x14)&~clear);};
 create(0x3266fc,0x1f0,300,0x2a4120,2);create(0x326700,0x18,5000,0x2a4138,0);create(0x326708,0x110,16,0x2a4148,0);create(0x326704,0xa0,600,0x2a4160,2);create(0x327de8,0x110,80,0x2a4170,3);create(0x32670c,0x30,1000,0x2a4190,2);create(0x326710,0x50,160,0x2a41a8,0);
}
// Debug00109F38; E-0099-W25. Actual routine retains ActiveTexture and VertexAnimator pools.
void eCloseSlotPools(Memory&m){for(auto global:{0x326700u,0x326704u,0x327de8u,0x32670cu,0x326710u})hp2::debug::bDeleteSlotPool(m,m.word(global));}
}

namespace port::race {
// Debug00103AB0/00114FC0/00103ED0; E-0099-W25. Complete authored header copy,
// distinct mutable stream/entries and shared original texture/material/animation records.
void eDuplicateSolid(Memory&m,const std::string&name,const std::string&new_name){
 const auto original=eFindSolid(m,hp2::frontend::bStringHash(name.c_str()));if(!original)return;
 if(new_name.size()>=24)throw std::out_of_range("Original duplicated solid name exceeds24bytes");
 const auto solid=m.allocate(0xc0);m.copy(solid,original,0xc0);
 const auto tail=m.word(0x32671c);m.word(tail,solid);m.word(solid+4,tail);m.word(0x32671c,solid);m.word(solid,0x326718);
 const auto size=m.word(original+0x3c),stream=m.allocate(size);m.copy(stream,m.word(original+0xb4),size);m.word(solid+0xb4,stream);m.word(0x326800,m.word(0x326800)+size);
 const auto count=m.signed_half(original+0x34);if(count<0)hp2::not_implemented("0x00103AB0","Negative original duplicated strip extent");
 const auto table=m.allocate(static_cast<unsigned>(count)*64);m.copy(table,m.word(original+0xb0),static_cast<unsigned>(count)*64);m.word(solid+0xb0,table);m.word(0x326800,m.word(0x326800)+static_cast<unsigned>(count)*64);
 for(int i=0;i<count;++i){const auto p=table+64*static_cast<unsigned>(i);m.word(p+8,m.word(p+8)+stream-m.word(original+0xb4));}
 m.half(solid+0x30,m.half(solid+0x30)|4);for(unsigned i=0;i<new_name.size();++i)m.byte(solid+16+i,static_cast<std::uint8_t>(new_name[i]));m.byte(solid+16+static_cast<unsigned>(new_name.size()),0);m.word(solid+8,hp2::frontend::bStringHash(new_name.c_str()));
}
// Debug00103C18/001150E0; E-0099-W25. Exact global flagged-copy cleanup.
void eDeleteAllDuplicatedSolids(Memory&m){for(auto p=m.word(0x326718);p!=0x326718;){const auto next=m.word(p);if(m.half(p+0x30)&4){
 const auto stream=m.word(p+0xb4);if(stream)m.release(stream);m.word(0x326800,m.word(0x326800)-m.word(p+0x3c));m.word(p+0xb4,0);
 const auto table=m.word(p+0xb0);if(table)m.release(table);m.word(0x326800,m.word(0x326800)-static_cast<unsigned>(m.signed_half(p+0x34))*64);m.word(p+0xb0,0);const auto prev=m.word(p+4);m.word(prev,next);m.word(next+4,prev);m.release(p);
 }p=next;}}
}

namespace port::race {
// Debug00103FA0; E-0099-W25. Original renderer-owned packed strip traversal,
// binding the real BodyDamage callback and copying its local XYZ carrier back.
void CallDamageVertexFunctionForEveryVertex(Memory&m,std::uint32_t model,std::uint32_t object){
 const auto solid=m.word(model+12);if(!solid)return;
 for(int strip=0;strip<m.signed_half(solid+0x34);++strip){const auto entry=m.word(solid+0xb0)+64*static_cast<unsigned>(strip),data=m.word(entry+8);const auto flags=m.half(entry+14);bool visible=false;
  for(int v=0;v<m.byte(entry+29);++v){const auto xyz=GetElementXYZ(m,data,v);std::array<float,3> vertex{{m.scalar(xyz[0]),m.scalar(xyz[1]),m.scalar(xyz[2])}};const auto rgba=GetElementRGBA(m,data,v);const bool changed=DamageVertexFunction(m,object,vertex,rgba,(flags&0x800)!=0);visible=visible||changed;for(unsigned axis=0;axis<3;++axis)m.scalar(xyz[axis],vertex[axis]);}
  if(flags&0x800)m.half(entry+14,visible?(m.half(entry+14)&0xfbff):(m.half(entry+14)|0x400));
 }
}
}

namespace port::race {
// Debug00107AB0, E-0099-W40. Exact eView stride80hex/global2C0C30.
std::uint32_t eGetView(Memory& m,int id){(void)m;return 0x2c0c30u+std::uint32_t(id)*0x80u;}
// Debug00107AF8, E-0099-W28. Actual current mode, no invented view selection.
int eGetCurrentViewMode(const Memory& m){return m.signed_word(0x32682c);}
}

#include "game_core/indep/inc/Camera.hpp"
#include "game_core/src/ecstasy/emathe.hpp"
namespace port::race {
// Debug00106E68, E-0099-W50. Original byte clear and intrusive-list reset.
void eView::Sub_00106E68(){for(unsigned i=0;i<0x80;++i)m_.byte(address_+i,0);m_.word(address_+0x3c,address_+0x3c);m_.word(address_+0x40,address_+0x3c);m_.scalar(address_+0x14,1);}
// Debug00105650, E-0099-W50. Original RenderTargets global and B0-byte stride.
std::uint32_t eGetRenderTarget(Memory& m,int id){(void)m;return 0x2c7410u+std::uint32_t(id)*0xb0u;}
// Debug001079B8, E-0099-W50. All four original assignments; no copied camera
// pose, player callback or substitute render-target allocation.
void eInitViews(Memory& m){
 const std::uint32_t cameras[]={0x2bf410,0x2bf5e0,0x2bf7b0,0x2bf980};
 for(unsigned i=0;i<4;++i){const auto view=eGetView(m,int(i));m.word(view,i);m.word(view+0x30,eGetRenderTarget(m,i==3?7:int(i)));m.word(view+0x38,cameras[i]);if(i<3)m.word(view+0x34,eGetRenderTarget(m,int(i)+3));if(!i)m.byte(view+4,1);}
}
void initialize_camera_view_state(Memory& m){
 if(m.vector(0x2d39d0)!=std::array<float,4>{{0,0,0,0}})throw std::logic_error("Port Camera/eView startup requires fresh pre-static Memory and may run only once");
 for(const auto camera:{0x2bf410u,0x2bf5e0u,0x2bf7b0u,0x2bf980u})hp2::debug::Camera(m,camera).Sub_001566E0();
 for(int i=0;i<10;++i)eView(m,eGetView(m,i)).Sub_00106E68();
 hp2::debug::Sub_0011C2E8(m);
 hp2::debug::eMathInit(m);
 eInitViews(m);
}
// Debug001073D8/001073F8, E-0099-W50. Actual list mutation order and mode update.
void eView::AttachCameraMover(std::uint32_t mover){const auto first=m_.word(address_+0x3c);m_.word(address_+0x3c,mover);m_.word(first+4,mover);m_.word(mover,first);m_.word(mover+4,address_+0x3c);eUpdateViewMode(m_);}
void eView::UnattachCameraMover(std::uint32_t mover){const auto previous=m_.word(mover+4),next=m_.word(mover);m_.word(previous,next);m_.word(next+4,previous);eUpdateViewMode(m_);}
// Debug00107B00, E-0099-W50. Two-player mode precedes rear-view mirror mode.
void eUpdateViewMode(Memory& m){const auto first=eGetView(m,1),second=eGetView(m,2),mirror=eGetView(m,3);const bool active=m.word(first+0x3c)!=first+0x3c;if(active&&m.word(second+0x3c)!=second+0x3c){m.word(0x32682c,3);return;}if(active&&m.word(mirror+0x3c)!=mirror+0x3c){m.word(0x32682c,2);return;}m.word(0x32682c,active?1:0);}
}

namespace port::race {
// Debug00103F10, E-0099-W38. Signed strip-count/texture byte, original raw
// 64-byte strip entries and8-byte texture table; only flags bit8 is mutated.
void eSolid::SetEnvironmentMapping(std::uint32_t hash,int on){
 const auto strips=m_.word(address_+0xb0),textures=m_.word(address_+0xb8);
 for(int i=0;i<m_.signed_half(address_+0x34);++i){const auto strip=strips+static_cast<unsigned>(i)*64;const auto texture=textures+static_cast<std::uint32_t>(static_cast<std::int32_t>(m_.signed_byte(strip))*8);if(m_.word(texture)!=hash)continue;const auto flags=m_.half(strip+14);if(on){if(!(flags&0x100))m_.half(strip+14,static_cast<std::uint16_t>(flags|0x100));}else if(flags&0x100)m_.half(strip+14,static_cast<std::uint16_t>(flags&0xfeff));}
}
// Debug0010CED8, E-0099-W38. Real nullable eModel->Solid delegation.
void eModel::SetEnvironmentMapping(std::uint32_t hash,int on){const auto solid=m_.word(address_+12);if(solid)eSolid(m_,solid).SetEnvironmentMapping(hash,on);}
// Debug0010F4B0/listing, E-0099-W60. Actual RC instant-replay ownership can
// suppress another player's view; ordinary construction does not require RC.
std::uint32_t GetPlayerView(Memory& m,int player){int view=player+1;const auto coordinator=m.word(0x327290);if(coordinator&&m.word(coordinator)==4){const auto replay=m.word(coordinator+0x2c);if(replay){if(player!=m.signed_word(replay+0x14))return 0;view=1;}}return eGetView(m,view);}
// Debug0010F588/listing, E-0099-W60. One actual eView.pBlendMask store.
void RadialBlurOff(Memory& m,std::uint32_t view){m.word(view+0x6c,0);}
// Debug0010F590, E-0099-W60. Keep the original nullable replay-view gate.
void RadialBlurOff(Memory& m,int player){if(const auto view=GetPlayerView(m,player))RadialBlurOff(m,view);}
// Debug0010F508/listing, E-0099-W63. Actual RC post-race gate and source
// view-indexed byte arrays; never replace the resident texture with a fake mask.
void RadialBlurOn(Memory& m,std::uint32_t view){const auto coordinator=m.word(0x327290);if(!coordinator||m.word(coordinator)!=7){const auto id=m.word(view);m.word(view+0x6c,m.word(0x326754));m.byte(0x3268e7+id,0xa0);m.byte(0x3268df+id,0);}}
// Debug0010F558, E-0099-W63. Preserve GetPlayerView's real nullable replay gate.
void RadialBlurOn(Memory& m,int player){if(const auto view=GetPlayerView(m,player))RadialBlurOn(m,view);}
// Debug0010F618/listing, E-0099-W63. Original255 factor, CVT.W.S and byte
// stores; no invented clamping or alternate screen-alpha representation.
void RadialBlurAlpha(Memory& m,int player,float center,float edge){const auto index=std::uint32_t(player);m.byte(0x3268e0+index,std::uint8_t(::port::driving::cvt_w_s(center*255)));m.byte(0x3268e8+index,std::uint8_t(::port::driving::cvt_w_s(edge*255)));}
void initialize_radial_blur_resource_state(Memory& m){const auto texture=hp2::debug::GetTextureInfo(m,0xab9a6e21,0,0);if(!texture)throw std::runtime_error("Port radial-blur resource component requires actual resident INGAMEB texture AB9A6E21");m.word(0x326754,texture);}
void release_radial_blur_resource_state(Memory& m){const auto texture=m.word(0x326754);if(texture){for(int i=0;i<10;++i)if(m.word(eGetView(m,i)+0x6c)==texture)throw std::logic_error("Port radial-blur resource is still borrowed by an active view");m.word(0x326754,0);}}
}

namespace port::race {
// Debug00115100/00115160, E-0099-W57. Original Ecstasy.hpp inline destructor
// used by WorldObject001CA7F8; existing real UnInit precedes pooled deletion.
void eModel::Sub_00115100(int flags){UnInit();if(flags&1)hp2::debug::SlotPool(m_,m_.word(0x326700)).Free(address_);}
}

// TASK-0099 E-0099-W65; original indep/src/ecstasy/ecstasye.cpp.
// Debug SHA256815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
namespace port::race {
std::uint32_t eModel::GetRigidAnimation()const{const auto solid=m_.word(address_+12);return solid?m_.word(solid+0xa8):0;}
int eModel::GetRigidAnimationTime()const{const auto start=m_.half(address_+0x16);if(!start)return -1;const auto elapsed=(m_.word(0x327d04)-start)&0x3fff;const auto anim=m_.word(m_.word(address_+12)+0xa8);const int duration=int(m_.signed_half(anim))*int(m_.signed_byte(anim+3));if(start&0x4000){if(!duration)hp2::not_implemented("0x0010DF14","Original rigid animation DIV1 duration zero executes BREAK7");return int(elapsed)%duration;}return int(elapsed)<duration?int(elapsed):-1;}
void eModel::SetRigidAnimationFrameNumber(int frame){m_.half(address_+0x16,std::uint16_t(((m_.word(0x327d04)-std::uint32_t(frame)*2)&0x3fff)|0x8000));}
}
namespace port::race {
// Debug0010CDA0 ->0010C130. Vec3 overload, no writes to caller padding W.
void eModel::GetBoundingBox(std::array<float,4>& low,std::array<float,4>& high)const{if(const auto solid=m_.word(address_+0xc))for(unsigned i=0;i<3;++i){low[i]=m_.scalar(solid+0x40+4*i);high[i]=m_.scalar(solid+0x50+4*i);}}
}
