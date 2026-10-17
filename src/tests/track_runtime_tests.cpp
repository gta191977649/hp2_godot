#include "game_core/port/track/runtime.hpp"
#include "game_core/src/ecstasy/elighte.hpp"
#include "game_core/port/driving/original_image.hpp"
#include "platform/ps2_asset_root.hpp"
#include <iostream>
#include <stdexcept>
#include <cmath>
using namespace port::track;
void require(bool v,const char* m){if(!v)throw std::runtime_error(m);}
int main(int argc,char** argv){try{
 auto scene=std::make_shared<Scene>();scene->entry.number=11;scene->world_lights_available=true;port::vehicle::Texture texture;texture.header.Field_0x6f=1;texture.header.alpha_bits=0x0a;scene->textures.push_back(texture);
 for(unsigned hash:{10,20,30}){port::render::VehicleSolid solid;solid.hash=hash;solid.surfaces.resize(2);scene->geometry.solids.push_back(solid);}
 Instance instance;instance.solid_hash=10;instance.flags=0x200;instance.lod_hashes={{10,20,30}};instance.far_clip_sizes={{10,3,0}};instance.position={{100,0,0}};instance.radius=2;instance.bbox_min={{99,-1,-1}};instance.bbox_max={{101,1,1}};instance.transform[0]=instance.transform[5]=instance.transform[10]=instance.transform[15]=1;scene->instances.push_back(instance);
 Section section;section.number=11;section.instance_count=1;SceneryNode node;node.minimum=instance.bbox_min;node.maximum=instance.bbox_max;node.children={0};section.nodes.push_back(node);scene->sections.push_back(section);
 Zone zone;zone.number=7;zone.sections={11};zone.perimeter={{{-10,-10}},{{10,-10}},{{10,10}},{{-10,10}}};scene->zones.push_back(zone);
 Runtime runtime;runtime.load(scene);require(runtime.poll_load(),"synchronous completion missing");FrameView view;view.clip_matrix[0]=view.clip_matrix[5]=view.clip_matrix[10]=.001f;view.clip_matrix[15]=1;view.focal_h=100;port::driving::Memory math;
 auto frame=runtime.prepare_frame(view,math);require(frame.zone==7&&frame.commands.size()==2&&frame.commands[0].lod==1,"Original pixel threshold/zone selection failed");require(frame.commands[0].surface==0&&frame.commands[1].surface==1&&frame.commands[1].original_order==1,"Surface submission order changed");
 view.focal_h=200;frame=runtime.prepare_frame(view,math);require(frame.commands.size()==2&&frame.commands[0].lod==0,"Near LOD selection failed");
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
 if(argc>1){std::string asset_root=argv[1];if(asset_root=="--assets"){const auto configured=hp2::platform::resolve_zzdata_root(".");if(!configured)return 77;asset_root=*configured;try{(void)port::driving::debug_image_path(".");}catch(const std::exception&){return 77;}}math=port::driving::read_debug_image(port::driving::debug_image_path("."));port::assets::DirectorySource source(asset_root);auto catalog=load_catalog(source);std::size_t count=0;for(const auto& entry:catalog){if(argc>2&&std::stoi(argv[2])!=entry.number)continue;runtime.load(source,entry);const auto& loaded=*runtime.resources();view.position={{0,0,0}};view.direction={{1,0,0}};view.focal_h=600;view.clip_matrix={};view.clip_matrix[0]=view.clip_matrix[5]=view.clip_matrix[10]=1.f/30000;view.clip_matrix[15]=1;auto real=runtime.prepare_frame(view,math);std::cout<<"TRACK_RUNTIME "<<entry.number<<" sections="<<loaded.sections.size()<<" zones="<<loaded.zones.size()<<" instances="<<loaded.instances.size()<<" draws="<<real.commands.size()<<"\n";++count;runtime.unload();}require(count>0,"No real track selected");}
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}return 0;}
