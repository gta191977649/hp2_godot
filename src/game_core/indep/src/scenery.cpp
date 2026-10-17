// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\scenery.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): GrandSceneryCullInfo::CullView [0x002B00A8]
// Diagnostic text (not a callable signature): GrandSceneryCullInfo::DoCulling() [0x002B00C8]
#include "game_core/indep/inc/Scenery.hpp"

#include "game_core/indep/src/scenery.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// Debug0014F9F8; E-0048-A05; original TU ..\indep\src\scenery.cpp.
// reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/track/0014f9f8.md.
port::driving::SolverMatrix CreateFacadeMatrix(port::driving::Memory& original_math,
    const std::array<float,3>& object_position,const std::array<float,3>& camera_position) {
    const auto angle=static_cast<std::uint16_t>(hp2::recovered::bATan(original_math,
        object_position[0]-camera_position[0],object_position[1]-camera_position[1]));
    port::driving::SolverMatrix matrix{};
    hp2::recovered::eRotateZ(original_math,matrix,hp2::recovered::bIdentity(),angle);
    for(std::size_t i=0;i<3;++i)matrix[12+i]=object_position[i];
    matrix[15]=1;
    return matrix;
}
// ELF 0x0014F340; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0014f340.md.
void VisibleZone::FindIndexTo() {
    hp2::not_implemented("0x0014F340", "VisibleZone::FindIndexTo (signature and full body unresolved)");
}
} // namespace hp2::recovered
// TASK-0051 E-0051-S01: original scenery class algorithms, explicit host resources.
#include "game_core/port/track/runtime.hpp"
#include "game_core/src/ecstasy/ecstasye.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace hp2::recovered {
// Retail0014F1E8 / Debug0014EE38, half-open crossing parity.
bool IsPointInPoly(const std::array<float,3>& point,const port::track::Zone& zone){
    bool inside=false;auto previous=zone.perimeter.size()-1;
    for(std::size_t i=0;i<zone.perimeter.size();previous=i++){
        const auto& p=zone.perimeter[i];const auto& q=zone.perimeter[previous];
        if((p[1]<=point[1]&&point[1]<q[1])||(q[1]<=point[1]&&point[1]<p[1]))
            if(point[0]<(q[0]-p[0])*(point[1]-p[1])/(q[1]-p[1])+p[0])inside=!inside;
    }return inside;
}
// Retail0014F2B0 / Debug0014EF00: successful original zone moves to list front.
const port::track::Zone* FindVisibleZone(const port::track::Scene& scene,const std::array<float,3>& position,std::vector<std::size_t>& order){
    for(auto it=order.begin();it!=order.end();++it)if(IsPointInPoly(position,scene.zones.at(*it))){const auto index=*it;order.erase(it);order.insert(order.begin(),index);return &scene.zones[index];}return nullptr;
}
// Retail0014FE18 / Debug0014FA68. Radius+4 pixel-size policy and first eligible LOD.
void ScenerySectionHeader::DrawAScenery(const port::track::Scene& scene,std::size_t index,const port::track::FrameView& view,unsigned visibility,port::driving::Memory& math,port::track::Frame& frame){
    const auto& object=scene.instances.at(index);++frame.considered_instances;
    if(visibility==1){visibility=ClassifyAABB(object.bbox_min,object.bbox_max,view.clip_matrix);if(!visibility)return;}
    if(object.flags&view.exclude_flags&0xff)return;
    std::array<float,3> delta{};float forward=0,squared=0;for(unsigned k=0;k<3;++k){delta[k]=object.position[k]-view.position[k];forward+=delta[k]*view.direction[k];squared+=delta[k]*delta[k];}
    const float radius=object.radius+4;int pixels=0;
    if(-radius<=forward){const float distance=std::sqrt(squared)-radius;float size=view.focal_h;if(radius<distance)size=radius*size/distance;if(!std::isfinite(size)||size>=2147483648.f||size< -2147483648.f)throw std::runtime_error("Invalid original scenery pixel-size conversion");pixels=static_cast<int>(size);}
    if(pixels<2)return;
    std::size_t solid=scene.geometry.solids.size();unsigned lod=0;
    for(;lod<3;++lod){if(pixels<object.far_clip_sizes[lod]||!object.lod_hashes[lod])continue;const auto found=std::find_if(scene.geometry.solids.begin(),scene.geometry.solids.end(),[&](const auto& s){return s.hash==object.lod_hashes[lod];});if(found==scene.geometry.solids.end())return;solid=std::size_t(found-scene.geometry.solids.begin());break;}
    if(solid==scene.geometry.solids.size())return;
    auto matrix=object.transform;const bool identity=object.flags&0x200;
    if(!identity){if(object.facades[lod])matrix=CreateFacadeMatrix(math,object.position,view.position);if(object.flags&view.exclude_flags&0x100)matrix[14]+=2;}
    GrandSceneryCullInfo::StuffScenery(scene,index,solid,lod,view,visibility,matrix,identity,forward,frame);
}
// Retail00150EB8 / Debug00150B08. Selected static draw-info lighting and model submission.
// Original DrawAScenery owns its matrix; StuffScenery selects shared or local lighting.
void GrandSceneryCullInfo::StuffScenery(const port::track::Scene& scene,std::size_t index,std::size_t solid,unsigned lod,const port::track::FrameView& view,unsigned visibility,const std::array<float,16>& matrix,bool identity,float depth,port::track::Frame& frame){
    const bool has_light_context=identity||eModel::IsLit(scene.geometry.solids.at(solid).strip_flags);
    std::array<float,28> light{};if(has_light_context)light=identity?scene.world_lights:elSetupLightContext(scene.light_packs,matrix);
    ++frame.visible_instances;
    port::track::DrawCommand command;command.instance=index;command.solid=solid;command.lod=lod;command.visibility=visibility;command.matrix=matrix;command.identity=identity;command.has_light_context=has_light_context;command.light_context=light;command.depth=depth;
    eModel::Render(scene,view,command,frame);
}
// Retail00150378 / Debug0014FFC8. Original 64-entry LIFO; leaf calls remain in child order.
void ScenerySectionHeader::TreeCull(const port::track::Scene& scene,const port::track::Section& section,const port::track::FrameView& view,port::driving::Memory& math,port::track::Frame& frame){
    if(section.nodes.empty())return;std::vector<std::pair<std::size_t,unsigned>> stack{{0,1}};std::size_t visits=0;
    while(!stack.empty()){const auto work=stack.back();stack.pop_back();if(++visits>section.nodes.size())throw std::runtime_error("Cyclic or shared original scenery tree");const auto& node=section.nodes.at(work.first);auto visibility=work.second;if(visibility==1)visibility=ClassifyAABB(node.minimum,node.maximum,view.clip_matrix);if(!visibility)continue;
        for(auto code:node.children)if(code<0){if(stack.size()==64)throw std::runtime_error("Original scenery stack capacity exceeded");stack.emplace_back(std::size_t(-int(code)),visibility);}else DrawAScenery(scene,section.instance_begin+std::size_t(code),view,visibility,math,frame);
    }
}
// Retail00150950/00150B10/00150D98; Debug001505A0/00150760/001509E8.
void GrandSceneryCullInfo::WhatSectionsShouldWeDraw(const port::track::Scene& scene,const port::track::FrameView& view,std::vector<std::size_t>& order,port::track::Frame& frame){
    const auto* zone=FindVisibleZone(scene,view.position,order);
    if(zone)frame.zone=zone->number;
    const auto append=[&](int number){if(frame.sections.size()==128)return;const auto found=std::find_if(scene.sections.begin(),scene.sections.end(),[&](const auto& s){return s.number==number;});if(number>=0&&found!=scene.sections.end())frame.sections.push_back(number);};
    if(zone){for(int number=90;number<100;++number)append(number);for(auto number:zone->sections)append(number);}else {for(auto it=scene.sections.rbegin();it!=scene.sections.rend();++it)if(it->number>=90&&it->number<100)append(it->number);for(const auto& section:scene.sections)if(section.number<90)append(section.number);}
}
// Retail00150B10 / Debug00150760. Section selection precedes tree traversal.
void GrandSceneryCullInfo::CullView(const port::track::Scene& scene,const port::track::FrameView& view,port::driving::Memory& math,port::track::Frame& frame){
    for(auto number:frame.sections){const auto found=std::find_if(scene.sections.begin(),scene.sections.end(),[&](const auto& s){return s.number==number;});ScenerySectionHeader::TreeCull(scene,*found,view,math,frame);}
}
}

namespace hp2::recovered {
// Retail00150D98 / Debug001509E8, single-view binding; no frame-buffer ownership.
void GrandSceneryCullInfo::DoCulling(const port::track::Scene& scene,const port::track::FrameView& view,std::vector<std::size_t>& order,port::driving::Memory& math,port::track::Frame& frame){WhatSectionsShouldWeDraw(scene,view,order,frame);CullView(scene,view,math,frame);}
}
