#include "game_core/port/race/nis_services.hpp"
// TASK-0099 E-0099-W51. Exact function definition markers indep/src/nis.cpp,
// live MCP FDR/SYMR corroborated. First CPP
// marker is included NISSoundTable.cpp, not the definition path of these bodies.
#include "game_core/indep/inc/NIS.hpp"

namespace hp2::debug {
// Debug001AEC68,E-0100-PA01. Playing number precedes queued/loading/loaded.
int NisManager::GetCurrentNisNumber()const{if(m_.word(address_+0x40))return m_.signed_word(address_+0x4c);const auto queued=m_.signed_word(address_+0x14);if(queued!=-1)return queued;const auto loading=m_.signed_word(address_+0x10);return loading!=-1?loading:m_.signed_word(address_+0xc);}
// TASK-0099 E-0099-W69. Original JR RA / MOVE V0,ZERO, no state consumed.
int NisManager::AllowDebugLooping()const{return 0;}
}
#include "game_core/port/race/car_render_resources.hpp"
#include "game_core/port/not_implemented.hpp"
#include <stdexcept>
namespace hp2::debug {
// Debug001B07E8/001B0830, E-0099-W55. Actual raw chunk binding and source
// unsigned quotient; whole authored A0-byte rows remain owned by the caller.
int LoaderNisInfo(::port::driving::Memory& m,std::uint32_t chunk){if(m.word(chunk)!=0x37010)return 0;m.word(0x327fcc,chunk+8);m.word(0x327fc8,m.word(chunk+4)/0xa0);return 1;}
int UnloaderNisInfo(::port::driving::Memory& m,std::uint32_t chunk){if(m.word(chunk)!=0x37010)return 0;m.word(0x327fcc,0);m.word(0x327fc8,0);return 1;}
// Debug001B0920, E-0099-W55. Real nullable table/type scan, no forced counts.
int GetNumNisInfo(::port::driving::Memory& m,int type){const auto table=m.word(0x327fcc);int count=0;if(table)for(int i=0;i<m.signed_word(0x327fc8);++i)if(m.signed_word(table+std::uint32_t(i)*0xa0+0x20)==type)++count;return count;}
// Debug001B0858/001B08B8/001B0A38,E-0100-PA01. Actual authored body hashes.
int NisSupportsPlayerBodyType(::port::driving::Memory& m,int number,std::uint32_t hash){if(!m.word(0x327fcc)||number>=m.signed_word(0x327fc8))return 0;if(number<0)hp2::not_implemented("0x001B0858","negative original NIS table index requires actual preceding memory");const auto row=m.word(0x327fcc)+unsigned(number)*0xa0;const int count=m.signed_word(row+0x40);if(count>8)hp2::not_implemented("0x001B0858","original NIS player body hash array exceeds eight entries");for(int i=0;i<count;++i)if(m.word(row+0x60+4*unsigned(i))==hash)return 1;return 0;}
int NisSupportsOtherBodyType(::port::driving::Memory& m,int number,std::uint32_t hash){if(!m.word(0x327fcc)||number>=m.signed_word(0x327fc8))return 0;if(number<0)hp2::not_implemented("0x001B08B8","negative original NIS table index requires actual preceding memory");const auto row=m.word(0x327fcc)+unsigned(number)*0xa0;const int count=m.signed_word(row+0x44);if(count>8)hp2::not_implemented("0x001B08B8","original NIS other body hash array exceeds eight entries");for(int i=0;i<count;++i)if(m.word(row+0x80+4*unsigned(i))==hash)return 1;return 0;}
int IsNisSuitable(::port::driving::Memory& m,int number,std::uint32_t player_hash,std::uint32_t other_hash,int other_number){return number!=other_number&&NisSupportsOtherBodyType(m,number,other_hash)&&NisSupportsPlayerBodyType(m,number,player_hash)?1:0;}
// Debug001B0A98/001B0AC8, E-0099-W55. Preserve source index tests and first
// matching type semantics; valid rows are source bytes, never cached mirrors.
std::uint32_t GetNisInfo(::port::driving::Memory& m,int number){const auto table=m.word(0x327fcc);return table&&number<m.signed_word(0x327fc8)?table+std::uint32_t(number)*0xa0:0;}
std::uint32_t GetNisInfo(::port::driving::Memory& m,int index,int type){const auto table=m.word(0x327fcc);if(table)for(int i=0;i<m.signed_word(0x327fc8);++i){const auto row=table+std::uint32_t(i)*0xa0;if(m.signed_word(row+0x20)==type){if(!index)return row;--index;}}return 0;}
// Debug001B1100. Original static list and XYZ-only hand offsets; preserve W.
// Debug001B1100, E-0099-W51.
void Sub_001B1100(::port::driving::Memory& m){for(auto p:{0x32b1d0u,0x32b1e0u})for(unsigned k=0;k<3;++k)m.scalar(p+4*k,0);m.word(0x327fd0,0x327fd0);m.word(0x327fd4,0x327fd0);}
// Debug001AE5C0. Preserve omitted fields, all four model ownership slots and
// original insertion order. Name is the port decoded original char argument.
// Debug001AE5C0, E-0099-W51.
void NisManager::Sub_001AE5C0(const std::string& name){if(name.size()>=32)throw std::out_of_range("Original NisManager name exceeds its32-byte field");auto& m=m_;const auto a=address_;m.scalar(a+0x70,0);m.vector(a+0x90,{{0,0,0,0}});m.vector(a+0xc0,{{0,0,0,0}});for(auto p:{0x80u,0xa0u,0xb0u,0xd0u})m.word(a+p,0);for(unsigned i=0;i<4;++i){m.word(a+0xe8+i*24,0);m.word(a+0xec+i*24,0);}for(auto p:{0x18u,0x1cu})m.word(a+p,0);for(auto p:{0xcu,0x10u,0x14u})m.word(a+p,0xffffffff);for(unsigned i=0;i<=name.size();++i)m.byte(a+0x20+i,i<name.size()?std::uint8_t(name[i]):0);const auto previous=m.word(0x327fd4);if(!previous)hp2::not_implemented("0x001B1100","NisManager static list initializer is required");m.word(a+0x4c,0xffffffff);m.word(previous,a);m.word(0x327fd4,a);m.word(a,0x327fd0);for(auto p:{0x40u,0x44u,0x48u,0x50u,0x54u,0x58u,0x5cu,0x60u,0x64u,0x68u})m.word(a+p,0);m.word(a+4,previous);InitSoundEvents();}
// Debug001AFF20. Return borrowed animations, then release real prop models.
// Debug001AFF20, E-0099-W51.
void NisManager::StopAnimating(){auto& m=m_;const auto a=address_;if(!m.word(a+0x5c))return;const auto player=m.word(a+0xa0);m.word(a+0x5c,0);if(const auto animation=m.word(player+0x4b4))m.word(animation+0x64,0);const auto other=m.word(a+0xd0);m.word(a+0x80,0);if(const auto animation=m.word(other+0x4b4))m.word(animation+0x64,0);m.word(a+0xb0,0);for(unsigned i=0;i<2;++i){::port::race::eModel(m,a+0xe0+i*24).UnInit();::port::race::eModel(m,a+0x110+i*24).UnInit();}}
// Debug001AE6F0. Loaded resource files remain a concrete required consumer.
// Debug001AE6F0, E-0099-W51.
void NisManager::Sub_001AE6F0(int flags){auto& m=m_;const auto a=address_;StopAnimating();if(m.signed_word(a+0xc)>=0){auto& services=::port::race::require_nis_services(m);if(!services.unload_resource)hp2::not_implemented("0x001AE6F0","actual NIS/CAM resource unloader is unbound");services.unload_resource(m.word(a+0x18));services.unload_resource(m.word(a+0x1c));}const auto next=m.word(a),previous=m.word(a+4);m.word(previous,next);m.word(next+4,previous);for(auto p:{0x128u,0x110u,0xf8u,0xe0u})::port::race::eModel(m,a+p).UnInit();if(flags&1)m.release(a);}
}

#include "game_core/indep/inc/Replay.hpp"
#include "game_core/indep/inc/World.hpp"
namespace hp2::debug {
// Debug001AE7D8; E-0099-W74; replay-frame/001ae7d8.json/type-NisManager.json.
void NisManager::DoSnapshot(std::uint32_t snapshot){
 ReplaySnapshot s(m_,snapshot);s.BeginSection(0x2ae278);if(m_.byte(snapshot+0x3d))StopAnimating();
 for(unsigned off:{0x44u,0x48u,0x4cu})s.MoveData(address_+off,4,4,1);
 World world(m_,m_.word(0x3272f0));world.DoPointerSnapshot(snapshot,address_+0x54);world.DoPointerSnapshot(snapshot,address_+0x58);
 s.MoveData(address_+0x40,1,4,1);s.MoveData(address_+0x50,1,4,1);s.MoveData(address_+0x64,1);s.MoveData(address_+0x68,1);
 if(m_.byte(snapshot+0x3d)&&m_.word(address_+0x40)){EnsureNisIsLoaded();StartAnimating();}
 s.EndSection();
}
}
#include "game_core/port/race/nis_services.hpp"
#include "game_core/indep/inc/Car.hpp"
#include "game_core/indep/src/carstate.hpp"
#include "game_core/indep/inc/CollisionBody.hpp"
#include "game_core/indep/src/track.hpp"
#include "game_core/eAnimation.hpp"
#include "game_core/port/race/physics_construction.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include <cstdio>
#include <algorithm>
namespace hp2::debug {
namespace {
std::string nis_source_text(const ::port::driving::Memory& m,std::uint32_t at,unsigned bound){std::string result;for(unsigned i=0;i<bound;++i){const auto c=m.byte(at+i);if(!c)return result;result.push_back(char(c));}hp2::not_implemented("0x001AEB18","original NIS source text lacks bounded terminator");}
}
void NisManager::NisLoadedCallback(){const auto loaded=m_.word(address_+0x10);const int queued=m_.signed_word(address_+0x14);m_.word(address_+0x10,0xffffffff);m_.word(address_+0xc,loaded);if(queued>=0){m_.word(address_+0x14,0xffffffff);Load(queued);}} //001AEAF0.
void NisManager::Load(int number){auto& m=m_;const auto a=address_;if(m.signed_word(a+0xc)==number||m.signed_word(a+0x10)==number)return;if(m.signed_word(a+0x10)>=0){m.word(a+0x14,unsigned(number));return;}auto& services=::port::race::require_nis_services(m);if(!services.load_resource||!services.unload_resource)hp2::not_implemented("0x001AEB18","actual NIS resource ownership services required");if(m.signed_word(a+0xc)>=0){services.unload_resource(m.word(a+0x18));services.unload_resource(m.word(a+0x1c));}const auto info=GetNisInfo(m,number);if(number<0||!info)hp2::not_implemented("0x001AEB18","original NIS number has no authored info row");const auto name=nis_source_text(m,info,32);(void)hp2::frontend::bStringHash(name.c_str());auto file=[&](std::uint32_t format){const auto fmt=nis_source_text(m,format,64);char buffer[32];const int n=std::snprintf(buffer,sizeof buffer,fmt.c_str(),name.c_str());if(n<0||n>=32)hp2::not_implemented("0x001AEB18","original NIS filename exceeds32 bytes");return std::string(buffer,unsigned(n));};m.word(a+0x10,unsigned(number));m.word(a+0x18,services.load_resource(file(0x2ae288)));m.word(a+0x1c,services.load_resource(file(0x2ae298)));
 // Synchronous PC I/O returns only after both original chunk consumers finish.
 // Deliver the exact source callback now; never publish synthetic loaded state.
 NisLoadedCallback();}
void NisManager::EnsureNisIsLoaded(){auto& m=m_;const auto a=address_;if(m.word(a+0xc)==m.word(a+0x4c))return;Load(m.signed_word(a+0x4c));while(m.word(a+0xc)!=m.word(a+0x4c)){auto& services=::port::race::require_nis_services(m);if(!services.service_resource_loading)hp2::not_implemented("0x001AEC10","actual queued NIS resource loading service required");services.service_resource_loading();}}
void NisManager::SetupCarsBeforeNIS(){auto& m=m_;auto& services=::port::race::require_nis_services(m);if(!services.physics)hp2::not_implemented("0x001AFC18","actual movement-mode physics dependencies required");for(const auto& pair:{std::pair<unsigned,unsigned>{0x54,0x68},{0x58,0x64}})if(const auto car=m.word(address_+pair.first)){if(m.word(car+0x24))m.word(address_+pair.second,m.word(car+0x24));hp2::recovered::CarState(m,car+0x40).ResetCarForSound();hp2::recovered::Car(m,car).SetMovementMode(0,*services.physics);CollisionBody(m,m.word(car+0x34)).DisableCollisions();}}
void NisManager::RestoreCarsAfterNIS(){auto& m=m_;auto& services=::port::race::require_nis_services(m);if(!services.physics)hp2::not_implemented("0x001AFCC0","actual movement-mode physics dependencies required");for(const auto& pair:{std::pair<unsigned,unsigned>{0x54,0x68},{0x58,0x64}})if(const auto car=m.word(address_+pair.first)){hp2::recovered::Car(m,car).SetMovementMode(m.signed_word(address_+pair.second),*services.physics);CollisionBody(m,m.word(car+0x34)).EnableCollisions();}}
void NisManager::StartAnimating(){auto& m=m_;const auto a=address_;m.word(a+0x5c,1);const auto info=GetNisInfo(m,m.signed_word(a+0x4c));if(!info)hp2::not_implemented("0x001AFD40","actual NIS info required");for(unsigned i=0;i<2;++i){::port::race::eModel(m,a+0xe0+24*i).Init(m.word(info+0x50+4*i));::port::race::eModel(m,a+0x110+24*i).Init(m.word(info+0x58+4*i));}const auto player=m.word(a+0x54),other=m.word(a+0x58);if(!player||!other)hp2::not_implemented("0x001AFD40","original NIS requires both source Cars");const auto position=player+0x500;const float height=TopologyCoordinate(m,player+0x520).GetElevation({{m.scalar(position),m.scalar(position+4),m.scalar(position+8)}});m.scalar(a+0x70,height);m.word(a+0xa0,player);const auto player_driver=m.word(player+0x4b4);if(player_driver){m.word(player_driver+0x64,1);m.word(a+0x80,m.word(player_driver+4));}else m.word(a+0x80,0);const float dz=-(m.scalar(position+8)-height);if(m.word(info+0x24)){const auto offset=m.word(player+0xc)+0xf0;for(unsigned i=0;i<3;++i)m.scalar(a+0x90+4*i,m.scalar(offset+4*i)+m.scalar(i==0?0x325844:0x325840));m.word(a+0x9c,0);}else m.vector(a+0x90,{{0,0,dz,0}});m.word(a+0xd0,other);const auto other_driver=m.word(other+0x4b4);if(other_driver){m.word(other_driver+0x64,1);m.word(a+0xb0,m.word(other_driver+4));}else m.word(a+0xb0,0);m.vector(a+0xc0,{{0,0,dz,0}});if(m.word(info+0x24))m.scalar(a+0xc0,m.scalar(a+0xc0)+m.scalar(a+0x90));m.word(a+8,0);}
void NisManager::Start(int number,int camera,std::uint32_t player,std::uint32_t other){auto& m=m_;const auto a=address_,info=GetNisInfo(m,number);if(number<0||!info)hp2::not_implemented("0x001AE8F8","original NIS start requires authored info");const auto now=m.word(0x327d04);m.word(a+0x44,now);m.word(a+0x50,unsigned(camera));m.word(a+0x40,1);m.word(a+0x4c,unsigned(number));m.word(a+0x54,player);m.word(a+0x58,other);m.word(a+0x48,now+m.word(info+0x2c)*2u);EnsureNisIsLoaded();SetupCarsBeforeNIS();StartAnimating();}
void NisManager::Stop(){if(!m_.word(address_+0x40))return;StopAnimating();RestoreCarsAfterNIS();for(auto field:{0x58u,0x40u,0x44u,0x48u,0x54u})m_.word(address_+field,0);for(auto field:{0x50u,0x4cu})m_.word(address_+field,0xffffffff);}
int NisManager::GetFrameNumber()const{return std::int32_t(m_.word(0x327d04)-m_.word(address_+0x44))/2;}
void NisManager::SetupForRendering(){auto& m=m_;const auto a=address_,player=m.word(a+0x54),p=player+0x500,info=GetNisInfo(m,m.signed_word(a+0x4c));m.scalar(a+0x70,TopologyCoordinate(m,player+0x520).GetElevation({{m.scalar(p),m.scalar(p+4),m.scalar(p+8)}}));if(m.word(info+0x24)){const auto offset=m.word(player+0xc)+0xf0;for(unsigned i=0;i<3;++i)m.scalar(a+0x90+4*i,m.scalar(offset+4*i)+m.scalar(i==0?0x325834:0x325830));m.word(a+0x9c,0);}const int frame=std::min(GetFrameNumber(),m.signed_word(info+0x2c)-1);if(const auto anim=m.word(a+0x80))eAnimation(m,anim).SetAnimation(m.word(info+0x34),float(frame));if(const auto anim=m.word(a+0xb0))eAnimation(m,anim).SetAnimation(m.word(info+0x38),float(frame));PlaySoundEvents(frame);}
}

#include "game_core/port/race/nis_camera_manager.inc"
#include "game_core/port/race/nis_override.inc"
