#include "game_core/port/track/runtime.hpp"
#include "game_core/port/track/projection.hpp"
#include "game_core/src/ecstasy/elighte.hpp"
#include "game_core/src/ecstasy/ecstasye.hpp"
#include "game_core/indep/inc/Scenery.hpp"
#include <tuple>
#include <chrono>
#include "game_core/port/driving/original_image.hpp"
#include "platform/ps2_asset_root.hpp"
#include <iostream>
#include <stdexcept>
#include <cmath>
using namespace port::track;
void require(bool v,const char* m){if(!v)throw std::runtime_error(m);}
// Compare every published field; padding bytes are deliberately irrelevant.
bool same_command(const DrawCommand&a,const DrawCommand&b){
 const auto base=[](const auto&v){return std::tie(v.instance,v.solid,v.surface,v.lod,v.visibility,v.matrix,v.light_context,v.identity,v.has_light_context,v.original_order,v.depth);};
 const auto effect=[](const auto&v){return std::tie(v.flags,v.matrix_pack,v.rendering_order,v.alpha_pass,v.rewrite_alpha,v.second_pass_320,v.environment_pass);};
 const auto texture=[](const auto&v){return std::tie(v.blend,v.deferred,v.texture_alpha,v.alpha_register,v.tex1_register,v.magnification_linear,v.minification_linear,v.repeat_u,v.repeat_v,v.maximum_mip_level,v.requires_lod_projection,v.additive);};
 const auto test=[](const auto&v){return std::tie(v.test_register,v.write_depth,v.alpha_test,v.alpha_reference,v.depth_test);};
 return base(a)==base(b)&&effect(a.effect)==effect(b.effect)&&texture(a.effect.texture)==texture(b.effect.texture)&&test(a.effect.test)==test(b.effect.test);
}
void same_frame(const Frame&a,const Frame&b){require(a.sections==b.sections&&a.zone==b.zone&&a.considered_instances==b.considered_instances&&a.visible_instances==b.visible_instances&&a.submitted_surfaces==b.submitted_surfaces&&a.commands.size()==b.commands.size(),"Indexed frame metadata differs from legacy");for(std::size_t i=0;i<a.commands.size();++i)require(same_command(a.commands[i],b.commands[i]),"Indexed ordered command differs from legacy");}
int main(int argc,char** argv){try{
 auto scene=std::make_shared<Scene>();scene->entry.number=11;scene->world_lights_available=true;port::vehicle::Texture texture;texture.header.Field_0x6f=1;texture.header.alpha_bits=0x0a;scene->textures.push_back(texture);
 for(unsigned hash:{10,20,30}){port::render::VehicleSolid solid;solid.hash=hash;solid.surfaces.resize(2);scene->geometry.solids.push_back(solid);}
 Instance instance;instance.solid_hash=10;instance.flags=0x200;instance.lod_hashes={{10,20,30}};instance.far_clip_sizes={{10,3,0}};instance.position={{100,0,0}};instance.radius=2;instance.bbox_min={{99,-1,-1}};instance.bbox_max={{101,1,1}};instance.transform[0]=instance.transform[5]=instance.transform[10]=instance.transform[15]=1;scene->instances.push_back(instance);
 Section section;section.number=11;section.instance_count=1;SceneryNode node;node.minimum=instance.bbox_min;node.maximum=instance.bbox_max;node.children={0};section.nodes.push_back(node);scene->sections.push_back(section);
 Zone zone;zone.number=7;zone.sections={11};zone.perimeter={{{-10,-10}},{{10,-10}},{{10,10}},{{-10,10}}};scene->zones.push_back(zone);
 Runtime runtime;runtime.load(scene);require(runtime.poll_load(),"synchronous completion missing");FrameView view;view.clip_matrix[0]=view.clip_matrix[5]=view.clip_matrix[10]=.001f;view.clip_matrix[15]=1;view.focal_h=100;port::driving::Memory math;
 auto frame=runtime.prepare_frame(view,math);require(frame.zone==7&&frame.commands.size()==2&&frame.commands[0].lod==1,"Original pixel threshold/zone selection failed");require(frame.commands[0].surface==0&&frame.commands[1].surface==1&&frame.commands[1].original_order==1,"Surface submission order changed");
  {auto sector_scene=std::make_shared<Scene>(*scene);sector_scene->instances.push_back(sector_scene->instances.front());auto extra=sector_scene->sections.front();extra.number=12;extra.instance_begin=1;sector_scene->sections.push_back(extra);
   Runtime sectors;sectors.load(sector_scene);auto selected=sectors.prepare_frame(view,math);
   require(selected.zone==7&&selected.sections==std::vector<int>{11}&&selected.commands.size()==2,"VisibleZone shortlist expanded to an unselected sector");
   for(const auto& command:selected.commands)require(command.instance==0,"Unselected sector entered native command output");
   Zone neighbour;neighbour.number=8;neighbour.sections={12};neighbour.perimeter={{{20,-10}},{{30,-10}},{{30,10}},{{20,10}}};sector_scene->zones.push_back(neighbour);
   sectors.load(sector_scene);auto across=view;across.position[0]=25;auto next=sectors.prepare_frame(across,math);require(next.zone==8&&next.sections==std::vector<int>{12},"Neighbouring VisibleZone did not select its own sector");
   selected=sectors.prepare_frame(view,math);require(selected.zone==7&&selected.sections==std::vector<int>{11},"Returning to a prior VisibleZone retained neighbour sectors");
   auto outside=view;outside.position[1]=100;auto fallback=sectors.prepare_frame(outside,math);
   require(fallback.zone==-1&&fallback.sections==std::vector<int>({11,12}),"Original no-zone section fallback changed");
  }

 view.focal_h=200;frame=runtime.prepare_frame(view,math);require(frame.commands.size()==2&&frame.commands[0].lod==0,"Near LOD selection failed");
 // E0058R01: exact policy boundaries, independent of Godot HLOD ranges.
 view.focal_h=31;require(runtime.prepare_frame(view,math).commands.empty(),"Below2 projected pixels must reject");
 view.focal_h=32;require(runtime.prepare_frame(view,math).commands.size()==2,"At2 projected pixels must retain eligible last slot");
 const auto saved_hashes=scene->instances[0].lod_hashes;const auto saved_sizes=scene->instances[0].far_clip_sizes;
 scene->instances[0].lod_hashes={{10,0,0}};scene->instances[0].far_clip_sizes={{3,0,0}};
 require(runtime.prepare_frame(view,math).commands.empty(),"Single model must disappear below its own FarClipSize");
 view.focal_h=50;require(runtime.prepare_frame(view,math).commands.size()==2,"Single model threshold equality must qualify");
 scene->instances[0].lod_hashes=saved_hashes;scene->instances[0].far_clip_sizes=saved_sizes;
 view.position[1]=100;auto fallback=runtime.prepare_frame(view,math);require(fallback.zone==-1&&fallback.sections==std::vector<int>{11},"Outside-zone loaded-section fallback changed");view.position[1]=0;
 view.focal_h=200;
 auto missing_lod=scene->geometry.solids[0];scene->geometry.solids[0].hash=999;require(runtime.prepare_frame(view,math).commands.empty(),"Missing selected model silently fell back to lower LOD");scene->geometry.solids[0]=missing_lod;
 view.direction={{-1,0,0}};require(runtime.prepare_frame(view,math).commands.empty(),"Behind-camera radius rejection failed");view.direction={{1,0,0}};
 scene->instances[0].flags|=8;require(runtime.prepare_frame(view,math).commands.empty(),"Original exclusion mask failed");scene->instances[0].flags&=~8;
 view.clip_matrix[12]=-5;require(runtime.prepare_frame(view,math).commands.empty(),"Tree trivial rejection failed");view.clip_matrix[12]=0;
 // Distinct local ambient must be queried at instance translation, not world zero.
 LightPack pack;hp2::recovered::vAABB lightnode;lightnode.PositionX=100;lightnode.ExtentX=lightnode.ExtentY=lightnode.ExtentZ=10;lightnode.NumChildren=-1;lightnode.ChildrenIndicies[0]=0;pack.tree.NodeArray.push_back(lightnode);hp2::recovered::eLight light;light.Type=0;light.Colour=0x00332211;light.PositionX=100;light.Size=10;light.FarStart=5;light.Intensity=1;pack.lights.push_back(light);auto matrix=instance.transform;matrix[12]=100;auto context=hp2::recovered::elSetupLightContext({pack},matrix);require(context[12]==17&&context[13]==34&&context[14]==51,"Per-instance ambient query failed");
 auto identity=instance.transform;require(hp2::recovered::elSetupLightContext({pack})==hp2::recovered::elSetupLightContext({pack},identity),"Per-instance identity lighting changed audited world context");
 scene->textures.clear();bool failed_frame=false;try{runtime.prepare_frame(view,math);}catch(const std::runtime_error&){failed_frame=true;}require(failed_frame,"Missing selected texture silently accepted");scene->textures.push_back(texture);require(runtime.prepare_frame(view,math).commands.size()==2,"Failed frame poisoned subsequent command preparation");
 runtime.unload();require(!runtime.ready(),"Unload retained resources");bool rejected=false;try{runtime.prepare_frame(view,math);}catch(const std::logic_error&){rejected=true;}require(rejected,"Unloaded frame query accepted");
 std::cout<<"TRACK_RUNTIME_UNIT_OK\n";
 if(argc>1){std::string asset_root=argv[1];if(asset_root=="--assets"){const auto configured=hp2::platform::resolve_zzdata_root(".");if(!configured)return 77;asset_root=*configured;try{(void)port::driving::debug_image_path(".");}catch(const std::exception&){return 77;}}math=port::driving::read_debug_image(port::driving::debug_image_path("."));require(original_focal_h(math,0)==10,"Original zero-FOV fallback changed");require(std::abs(original_focal_h(math,0x4000)-256.f)<.01f,"Original global512 focal projection changed");port::assets::DirectorySource source(asset_root);auto catalog=load_catalog(source);std::size_t count=0;for(const auto& entry:catalog){if(argc>2&&std::stoi(argv[2])!=entry.number)continue;runtime.load(source,entry);const auto& loaded=*runtime.resources();view.position={{0,0,0}};view.direction={{1,0,0}};view.focal_h=600;view.clip_matrix={};view.clip_matrix[0]=view.clip_matrix[5]=view.clip_matrix[10]=1.f/30000;view.clip_matrix[15]=1;std::vector<std::size_t> legacy_zones(loaded.zones.size());std::iota(legacy_zones.begin(),legacy_zones.end(),0);std::vector<std::uint32_t> legacy_chains;
 for(unsigned pass=0;pass<3;++pass){view.position[0]=float(pass)*25;auto real=runtime.prepare_frame(view,math);auto linear=hp2::recovered::eDisplayFrame(loaded,view,legacy_zones,math,&legacy_chains);same_frame(real,linear);}
 auto begin=std::chrono::steady_clock::now();Frame real;for(unsigned repeat=0;repeat<20;++repeat)real=runtime.prepare_frame(view,math);auto elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count()/20;
 std::cout<<"TRACK_RUNTIME_PREPARE_MS "<<entry.number<<" "<<elapsed<<"\n";std::cout<<"TRACK_RUNTIME "<<entry.number<<" sections="<<loaded.sections.size()<<" zones="<<loaded.zones.size()<<" instances="<<loaded.instances.size()<<" draws="<<real.commands.size()<<"\n";++count;runtime.unload();}require(count>0,"No real track selected");}
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}return 0;}
