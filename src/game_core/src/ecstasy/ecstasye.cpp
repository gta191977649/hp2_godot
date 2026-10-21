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
