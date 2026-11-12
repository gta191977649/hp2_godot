// TASK-0099 E-0099-W40. Exact original ..\indep\src\soundcollision.cpp
// ownership corroborated by live MCP FDR/SYMR/marker/path reads.
#include "soundcollision.hpp"
#include "game_core/port/not_implemented.hpp"
#include "game_core/indep/src/collision.hpp"
#include "game_core/inc/bSlotPool.hpp"
#include "game_core/indep/inc/Sound3d.hpp"
#include "game_core/port/race/car_render_resources.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include "game_core/port/driving/projections.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/indep/src/sound.hpp"
#include "game_core/indep/src/sound3dsample.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>
namespace {
std::string port_collision_text(const ::port::driving::Memory& m,std::uint32_t at){std::string value;for(unsigned i=0;i<4096;++i){const auto c=m.byte(at+i);if(!c)return value;value.push_back(char(c));}throw std::out_of_range("Unterminated original collision sound string");}
void port_require_collision_list(const ::port::driving::Memory& m){if(!m.word(0x3285d0)||!m.word(0x3285d4))hp2::not_implemented("0x001E2348","original collision sound static free list is not initialized");}
void port_add_collision_free(::port::driving::Memory& m,std::uint32_t sample){port_require_collision_list(m);const auto node=sample+0xbc,first=m.word(0x3285d0);m.word(0x3285d0,node);m.word(first+4,node);m.word(node+4,0x3285d0);m.word(node,first);}
void port_delete_collision_sample(::port::driving::Memory& m,std::uint32_t sample){const auto table=m.word(sample+0xb8),target=m.word(table+12);const auto adjusted=sample+std::uint32_t(std::int32_t(m.signed_half(table+8)));if(target!=0x001e2360){char at[11];std::snprintf(at,sizeof(at),"0x%08X",target);hp2::not_implemented(at,"original CollisionSample virtual destructor target is not implemented");}hp2::debug::CollisionSample(m,adjusted).Sub_001E2360(3);}
std::uint32_t port_new_collision_sample(::port::driving::Memory& m,std::uint32_t source){const auto pool=m.word(0x3285c8);if(!pool)hp2::not_implemented("0x001E0D00","original CollisionSounds sample pool is not registered");const auto sample=hp2::debug::bOMalloc(m,pool);hp2::debug::bSample(m,sample).Sub_0026B2A0();m.word(sample+0xb8,0x2b1160);hp2::debug::CollisionSample(m,sample).Initialize(source);return sample;}
float port_collision_age(const ::port::driving::Memory& m,std::uint32_t collision,std::uint32_t coefficient){return float(std::int32_t(m.word(0x327cf8)-m.word(collision+8)))*m.scalar(coefficient);}
}
namespace hp2::debug {
// Debug001E2348, E-0099-W40. Only the observed process-static list fields.
void Sub_001E2348(::port::driving::Memory& m){m.word(0x3285d0,0x3285d0);m.word(0x3285d4,0x3285d0);}
// Debug001E0C18, E-0099-W40. Original nullable libraries and round-robin state.
std::uint32_t GetCollisionSoundType(::port::driving::Memory& m,std::uint32_t collision){const auto terrain=Collision(m,collision).GetTerrainType();std::uint32_t library;
 if(terrain>14&&terrain<17)library=0x311e68;else{auto level=::port::driving::cvt_w_s((m.scalar(collision+0x50)*m.scalar(0x325b78))*3.0f);if(level<0)level=0;else if(level>=3)level=2;library=0x311e78+std::uint32_t(level)*16;}
 const auto index=(m.signed_word(library+12)+1)%3;m.word(library+12,std::uint32_t(index));return m.word(library+std::uint32_t(index)*4);
}
// Debug001E0D00, E-0099-W40. Register creates a pool; it does not load libraries.
void CollisionSounds::Register(::port::driving::Memory& m){m.word(0x3285fc,0);if(!m.word(0x3285c8))m.word(0x3285c8,bNewSlotPool(m,0xc4,40,0x2b1048,0x10));if(!m.word(0x3285d8)){m.word(0x3285d8,hp2::frontend::bStringHash(port_collision_text(m,0x2b1060).c_str()));m.word(0x3285dc,hp2::frontend::bStringHash(port_collision_text(m,0x2b1070).c_str()));}}
// Debug001E0D68, E-0099-W40. Every library pointer is a real InGame source.
void CollisionSounds::LoadSampleTable(::port::driving::Memory& m){m.word(0x3285e0,1);const std::uint32_t names[]={0x2b1088,0x2b1098,0x2b10b0,0x2b10c0,0x2b10d0};const std::uint32_t libraries[]={0x311e78,0x311e88,0x311e98,0x311e68,0x311e58};
 for(unsigned n=0;n<5;++n){auto name=port_collision_text(m,names[n])+port_collision_text(m,0x2b1080);if(name.empty()||name.size()>=32)throw std::out_of_range("Original collision sample template outside32-byte field");::port::driving::Memory::Temporary text(m,32);for(unsigned i=0;i<3;++i){name.back()=char('1'+i);std::memcpy(m.view(text.address(),name.size()+1).data,name.c_str(),name.size()+1);const auto sample=GetInGameSample(m,text.address());if(!sample)hp2::not_implemented("0x001E0D68","required authored collision library sample is absent");m.word(libraries[n]+i*4,sample);bSample(m,sample).SetCategory(0);}m.word(libraries[n]+12,0);}
 for(const auto pair:{std::array<std::uint32_t,2>{{0x3285f8,0x2b10e0}},std::array<std::uint32_t,2>{{0x3285f4,0x2b10f8}},std::array<std::uint32_t,2>{{0x3285f0,0x2b1108}}}){const auto sample=GetInGameSample(m,pair[1]);if(!sample)hp2::not_implemented("0x001E0D68","required authored scrape/glass source is absent");m.word(pair[0],sample);bSample(m,sample).SetCategory(0);}
}
// Debug001E1118/listing, E-0099-W40. XYZ squared sum order is (x+y)+z.
float GetDistanceToCamera(::port::driving::Memory& m,std::uint32_t point){const auto distance=[&](int view){const auto camera=m.word(::port::race::eGetView(m,view)+0x38);if(!camera)hp2::not_implemented("0x001E1118","actual eView camera pose required by collision sound is not initialized");const auto x=m.scalar(camera+0x40)-m.scalar(point),y=m.scalar(camera+0x44)-m.scalar(point+4),z=m.scalar(camera+0x48)-m.scalar(point+8);return std::sqrt((x*x+y*y)+z*z);};auto result=distance(1);if(m.signed_word(0x311fa8)>1){const auto other=distance(2);if(other<result)result=other;}return result;}
// Debug001E11E8/001E10C8, E-0099-W40. Distance is evaluated BEFORE zero impulse.
float CollisionSounds::CalcCollisionVolume(::port::driving::Memory& m,std::uint32_t collision){const auto distance=GetDistanceToCamera(m,collision+0x10);if(!(distance<35))return 0;auto volume=(m.scalar(collision+0x50)*m.scalar(0x325b80))*m.scalar(0x325b84);if(volume>1)volume=1;auto attenuation=35.0f/distance;if(attenuation>1)attenuation=1;return attenuation*volume;}
float CollisionSounds::CalcCollisionScrapeVolume(::port::driving::Memory& m,std::uint32_t collision){auto volume=(CalcCollisionVolume(m,collision)*m.scalar(collision+0x30))*m.scalar(0x325b7c);if(volume>1)volume=1;return volume;}
// Debug001E2358/001E2360/001E2400, E-0099-W40. Node destructor flags0 is empty;
// subclass delete returns the wholeC4 object to its own pool, not bSample's pool.
void CollisionSample::Initialize(std::uint32_t source){bSample::Initialize(source);}
void CollisionSample::Sub_001E2360(int flags){m_.word(address_+0xb8,0x2b1160);Sub_0026B3F8(0);if(flags&1){const auto pool=m_.word(0x3285c8);if(!pool)hp2::not_implemented("0x001E2400","original CollisionSample pool is not registered");SlotPool(m_,pool).Free(address_);}}
// Debug001E1288, E-0099-W40. Exact age/type/source gates and original ordering:
// assign Scrape before volume; SetAzimuth before Play; random pitch afterward.
void CollisionSounds::StartScraping(::port::driving::Memory& m,std::uint32_t collision){if(!(m.scalar(0x325b8c)<=port_collision_age(m,collision,0x325b88)))return;const auto terrain=Collision(m,collision).GetTerrainType();const auto source=terrain>14&&terrain<17?m.word(0x3285f4):terrain==17?m.word(0x3285f8):0;if(!source)return;const auto sample=port_new_collision_sample(m,source);m.word(collision+0x5c,sample);const auto volume=CalcCollisionScrapeVolume(m,collision);if(m.word(0x326f40)){bSample sound(m,sample);sound.SetAzimuth(Sound3d::ComputeAzimuth(m,collision+0x20));sound.Play(volume*m.scalar(0x325b90),0,0);sound.SetPitch(hp2::recovered::bRandom(m,0x327eec,m.scalar(0x325b94))+m.scalar(0x325b98));}}
// Debug001E13C0, E-0099-W40.
void CollisionSounds::UpdateSample(::port::driving::Memory& m,std::uint32_t collision){if(const auto sample=m.word(collision+0x5c)){const auto volume=(Sound3d::ComputeVolume(m,collision+0x20,200)*CalcCollisionScrapeVolume(m,collision))*m.scalar(0x325b9c);const auto azimuth=Sound3d::ComputeAzimuth(m,collision+0x20);bSample sound(m,sample);sound.SetVolume(volume);sound.SetAzimuth(azimuth);}}
}
namespace hp2::debug::collision_sound {
// Debug001E1768/001E1770/001E1778 wrappers, E-0099-W40.
void Register(::port::driving::Memory& m){CollisionSounds::Register(m);}void UnRegister(::port::driving::Memory& m){CollisionSounds::UnRegister(m);}void Update(::port::driving::Memory& m){CollisionSounds::Update(m);}
// Debug001E1788, E-0099-W40. Detach list nodes before actual destruction.
void Reset(::port::driving::Memory& m){port_require_collision_list(m);while(m.word(0x3285d0)!=0x3285d0){const auto node=m.word(0x3285d0),next=m.word(node),previous=m.word(node+4);m.word(previous,next);m.word(next+4,previous);port_delete_collision_sample(m,node-0xbc);}}
// Debug001E1838, E-0099-W40. Ctor calls this BEFORE stamping CollisionTime;
// never assume age0 or bypass the real type/library/volume ordering.
std::uint32_t CreateSample(::port::driving::Memory& m,std::uint32_t collision){if(!(m.scalar(0x325ba8)<=port_collision_age(m,collision,0x325ba4)))return 0;const auto source=GetCollisionSoundType(m,collision);if(!source)return 0;const auto volume=CollisionSounds::CalcCollisionVolume(m,collision);if(!(m.scalar(0x325bac)<volume))return 0;const auto sample=port_new_collision_sample(m,source);if(m.word(0x326f40)){const auto pitch=(hp2::recovered::bRandom(m,0x327eec,m.scalar(0x325bb0))+0.5f)-1.0f;bSample sound(m,sample);sound.Play(volume*m.scalar(0x325bb4),0,pitch);sound.SetAzimuth(Sound3d::ComputeAzimuth(m,collision+0x20));}return sample;}
// Debug001E1970, E-0099-W40. Playing one-shots are deferred; nonplaying scrape
// deliberately remains untouched, matching the original conditional cleanup.
void ReleaseSample(::port::driving::Memory& m,std::uint32_t collision){if(const auto sample=m.word(collision+0x58)){if(bSample(m,sample).IsPlaying())port_add_collision_free(m,sample);else port_delete_collision_sample(m,sample);}if(const auto scrape=m.word(collision+0x5c);scrape&&bSample(m,scrape).IsPlaying()){bSample(m,scrape).Fade(0,50);port_add_collision_free(m,scrape);}}
// Debug001E1D80, E-0099-W40. Genuine original sound gate, never forced by port.
void PlayScrape(::port::driving::Memory& m,std::uint32_t collision){if(m.word(0x326f40)&&!m.word(collision+0x5c))CollisionSounds::StartScraping(m,collision);}
// Debug001E1C20, E-0099-W40. Original glass source and concrete3D one-shot.
void PlayGlass(::port::driving::Memory& m,std::uint32_t position){if(m.word(0x326f40))sound3d_oneshot::Play(m,m.word(0x3285f0),position,m.scalar(0x325bc8),1);}
}
namespace hp2::debug {
// Debug001E1038, E-0099-W40. Whole deferred list then actual pool teardown.
void CollisionSounds::UnRegister(::port::driving::Memory& m){m.word(0x3285fc,0);collision_sound::Reset(m);if(const auto pool=m.word(0x3285c8)){bDeleteSlotPool(m,pool);m.word(0x3285c8,0);}}
// Debug001E1468, E-0099-W40. Actual lazy-source loading, deferred cleanup and
// World collision-list iteration. No invented loading-complete callback.
void CollisionSounds::Update(::port::driving::Memory& m){const auto world=m.word(0x3272f0);if(!world)return;if(!m.word(0x3285fc)&&IsSoundRegionLoadingComplete(m)){m.word(0x3285fc,1);LoadSampleTable(m);}port_require_collision_list(m);auto node=m.word(0x3285d0);while(node!=0x3285d0){const auto sample=node-0xbc,next=m.word(node);bSample sound(m,sample);if(!sound.IsPlaying()||sound.GetVolume()<=m.scalar(0x325ba0)){const auto previous=m.word(node+4);m.word(previous,next);m.word(next+4,previous);port_delete_collision_sample(m,sample);}node=next;}
 const auto head=world+0x10;for(auto collision=m.word(head);collision!=head;collision=m.word(collision)){if(m.word(collision+0x5c))UpdateSample(m,collision);if(!m.word(collision+0x58))m.word(collision+0x58,collision_sound::CreateSample(m,collision));}}
}
