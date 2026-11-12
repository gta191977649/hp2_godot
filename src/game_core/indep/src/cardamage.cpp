// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\cardamage.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): CarDamage::AddDamage() [0x002B65E0]
#include "game_core/indep/inc/CarDamage.hpp"

// Debug authority SHA-256815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
#include "game_core/port/race/damage_resources.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include "game_core/port/not_implemented.hpp"
#include <string>
#include <cmath>
namespace hp2::debug {
// Debug001B2940; E-0099-W22; exact original cardamage.cpp owner.
void CarDamage::Sub_001B2940(std::uint32_t car){
 BodyDamage(m_,address_+8).Sub_001B2030();
 const auto info=m_.word(car+0xc);std::string name;
 for(auto p=info+0x20;m_.byte(p);++p)name.push_back(static_cast<char>(m_.byte(p)));
 m_.word(address_+0xcc,0);m_.word(address_+0xd0,0);m_.word(address_,car);
 m_.word(address_+4,hp2::frontend::bStringHash(name.c_str()));
 for(int i=14;i>=0;--i)m_.byte(address_+0xd4+static_cast<unsigned>(i),0xff);
}
// Debug001B2A28; E-0099-W22; real original marker search and sector projection.
void CarDamage::InitSectorPositions(){
 for(unsigned i=0;i<15;++i){
  m_.byte(address_+0xd4+i,0xff);
  const auto marker=eFindPositionMarker(m_,m_.word(0x30b290+0x20*i+4),m_.word(address_+4));
  if(marker)m_.byte(address_+0xd4+i,static_cast<std::uint8_t>(BodyDamage(m_,address_+8).GetSectorNumber(marker+0x10)));
 }
}
// Debug001B29C8; E-0099-W22/W38. Actual breakable restoration followed by
// embedded BodyDamage repair and unlink; source does not free this here.
void CarDamage::Sub_001B29C8(){
 m_.word(address_+0xcc,0);
 ApplyDamage();
 BodyDamage(m_,address_+8).Sub_001B20A0();
}
}

namespace hp2::debug {
// Debug001B2B40, E-0099-W38. Exact original gates and real loaded BodyDamage
// sector projection; no substitute damage accumulation or silent enabled path.
void CarDamage::AddDamage(std::uint32_t position,float intensity){if(!m_.word(0x311f78+0x40))return;if(!std::isfinite(intensity))hp2::not_implemented("0x001B2B78","Exceptional original intensity comparison");if(intensity<m_.scalar(0x325860))return;const int sector=BodyDamage(m_,address_+8).GetSectorNumber(position);if(sector<0)return;
 AddDamageToSector(sector,intensity,intensity);float amount=intensity;for(int i=1;i<5;++i){amount*=0.5f;AddDamageToSector((sector+i)&31,amount,intensity);AddDamageToSector((sector-i)&31,amount,intensity);}ApplyDamage();
}
}

namespace hp2::debug {
// Debug001B2C30, E-0099-W38. Original signed-sector lookup,4 threshold bins,
// packed2-bit item values and three authored per-CarType unbreakable masks.
std::uint32_t CarDamage::RecalculateBreakableDamage(){std::uint32_t result=0;const BodyDamage body(m_,address_+8);
 for(unsigned item=0;item<15;++item){const int sector=m_.signed_byte(address_+0xd4+item);if(sector<0)continue;const float amount=body.GetVisibleDamageAmount(sector);const auto info=0x30b290+item*32;unsigned level=3;if(amount<m_.scalar(info+8+12)){for(level=2;level>0;--level)if(m_.scalar(info+8+level*4)<=amount)break;}result|=level<<(item*2);}
 const auto car=m_.word(address_),type=m_.word(m_.word(car+0x10));for(unsigned i=0;i<3;++i)if(m_.word(0x30b470+i*8)==type)result&=~m_.word(0x30b474+i*8);return result;
}
}

#include "game_core/indep/inc/Car.hpp"
#include "game_core/indep/src/soundcollision.hpp"
#include <cmath>
namespace hp2::debug {
// Debug001B2D58, E-0099-W38. Actual threshold rollback, packed breakable
// transitions, glass producer and raw Car emitter invocation, in source order.
void CarDamage::AddDamageToSector(int sector,float amount,float intensity){
 if(sector<0||sector>=32)throw std::out_of_range("Original damage sector");
 BodyDamage body(m_,address_+8);const float previous=static_cast<float>(m_.byte(address_+0x10+static_cast<unsigned>(sector)))*0.0078125f;
 float next=previous+amount;if(next<0)next=0;if(next>1)next=1;if(!std::isfinite(next))hp2::not_implemented("0x001B2D58","Exceptional original damage clamp operand");if(previous==next)return;
 body.SetDamage(sector,next);const auto bits=RecalculateBreakableDamage();if(bits==m_.word(address_+0xcc))return;
 if(intensity<m_.scalar(0x325864)){body.SetDamage(sector,previous);return;}
 const auto car=m_.word(address_);for(unsigned item=0;item<15;++item){const auto level=(bits>>(item*2))&3,old=(m_.word(address_+0xcc)>>(item*2))&3;if(level==old)continue;
  if(level==1)collision_sound::PlayGlass(m_,car+0x60);const auto effect=m_.signed_word(0x30b290+item*32+0x1c);
  if(effect)hp2::recovered::Car(m_,car).TriggerEffect(effect,m_.scalar(0x30b488+level*4),0,0);
 }
 m_.word(address_+0xcc,bits);
}
}

#include "game_core/port/race/car_render_resources.hpp"
namespace hp2::debug {
// Debug001B2F38, E-0099-W38. Model callback precedes breakable mapping and
// packed applied state; the original texture producer runs once after changes.
void CarDamage::ApplyDamage(){BodyDamage body(m_,address_+8);body.ApplyDamage();if(m_.word(address_+0xcc)==m_.word(address_+0xd0))return;
 for(unsigned item=0;item<15;++item){const unsigned shift=item*2,level=(m_.word(address_+0xcc)>>shift)&3;auto applied=m_.word(address_+0xd0);if(level==((applied>>shift)&3))continue;const auto info=0x30b290+item*32;if(m_.word(info+0x18)){body.SetEnvironmentMapping(m_.word(info+4),level==0);applied=m_.word(address_+0xd0);}m_.word(address_+0xd0,(applied&~(3u<<shift))|(level<<shift));}
 ::port::race::CarRenderInfo(m_,m_.word(m_.word(address_)+0x4b0)).UpdateCarReplacementTextures();
}
}

// Debug00124EB8, E-0099-W68; original inline ../indep/inc/CarDamage.hpp.
// SRLV consumes low5 of doubled item, then original two-bit damage field mask.
namespace hp2::debug {
int CarDamage::GetDamageAmount(std::uint32_t bits,int item)const{return int((bits>>((std::uint32_t(item)*2)&31))&3);}
}

#include "game_core/indep/inc/Replay.hpp"
namespace hp2::debug {
// Debug001B2AC8; TASK-0099 E-0099-W74; replay-frame/001b2ac8.json/type-CarDamage.json.
void CarDamage::DoSnapshot(std::uint32_t snapshot){
 ReplaySnapshot s(m_,snapshot);s.BeginSection(address_,0x2ae390);BodyDamage(m_,address_+8).DoSnapshot(snapshot);
 s.MoveData(address_+0xcc,4);if(m_.byte(snapshot+0x3d))ApplyDamage();s.EndSection();
}
}

namespace hp2::debug {
// Debug001B2F08; E-0099-W81; replay-events/001b2f08.json; no injected effects/RNG.
void CarDamage::RethinkDamage(){m_.word(address_+0xcc,RecalculateBreakableDamage());ApplyDamage();}
}


namespace hp2::debug {
// Debug001B30B0,E-0110-P01; original cardamage.cpp. Actual body repair and
// breakable recalculation/application, not a visual damage-counter reset.
void CarDamage::SetDamageToZero(){BodyDamage(m_,address_+8).RepairAllDamage();RethinkDamage();}
}

namespace hp2::debug {
// Debug001B3058, E-0112-COP02. Exact original cardamage.cpp loop and tailcall.
// Existing AddDamageToSector retains original thresholds/effects/packed state;
// RethinkDamage applies actual loaded model damage and replacement textures.
void CarDamage::SetDamageToFull(){for(int sector=0;sector<32;++sector)AddDamageToSector(sector,1.f,m_.scalar(0x325868));RethinkDamage();}
}
