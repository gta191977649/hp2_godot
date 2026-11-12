// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/src/bodydamage.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x001B4D48; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001b4d48.md.
void BodyDamage::SetDamage() {
    hp2::not_implemented("0x001B4D48", "BodyDamage::SetDamage (signature and full body unresolved)");
}

// ELF 0x001B4E10; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001b4e10.md.
void BodyDamage::GetSectorNumber() {
    hp2::not_implemented("0x001B4E10", "BodyDamage::GetSectorNumber (signature and full body unresolved)");
}

// ELF 0x001B4F10; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001b4f10.md.
void BodyDamage::GetDamageAmount() {
    hp2::not_implemented("0x001B4F10", "BodyDamage::GetDamageAmount (signature and full body unresolved)");
}

// ELF 0x001B50C8; E-0042-C133; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001b50c8.md.
void BodyDamage::GetVisibleTotalDamage() {
    hp2::not_implemented("0x001B50C8", "BodyDamage::GetVisibleTotalDamage (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/port/race/damage_resources.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include "game_core/port/driving/projections.hpp"
#include <algorithm>
namespace hp2::debug {
// Debug001B2030; E-0099-W22. Name parameter is unused, models retain original padding.
void BodyDamage::Sub_001B2030(){
 constexpr std::uint32_t list=0x327fe0;const auto tail=m_.word(list+4);
 m_.word(tail,address_);m_.word(address_+4,tail);m_.word(list+4,address_);m_.word(address_,list);
 for(unsigned i=0;i<32;++i)m_.byte(address_+8+i,0);
 for(unsigned i=0;i<32;++i)m_.byte(address_+0x2a+i,0);
 m_.half(address_+0x28,0);m_.half(address_+0x4a,0);m_.word(address_+0x4c,0);m_.word(address_+0xc0,0);
}
// Debug001B1C78; E-0099-W22.
std::uint32_t FindBodyDamageInfo(Memory&m,std::uint32_t hash){
 for(auto p=m.word(0x327fd8);p!=0x327fd8;p=m.word(p))if(m.word(p+0x28)==hash)return p;
 return 0;
}
// Debug001B2168; E-0099-W22. Original missing-name branch publishes null.
void BodyDamage::SetALevelBodyDamageInfo(const char*name){m_.word(address_+0xc0,FindBodyDamageInfo(m_,hp2::frontend::bStringHash(name)));}
// Debug001B22A0; E-0099-W22. Preserve unbounded normalized coordinates for side selection.
int BodyDamage::GetSectorNumber(std::uint32_t position){
 const auto info=m_.word(address_+0xc0);if(!info)return -1;
 const float minx=m_.scalar(info+0x2c),miny=m_.scalar(info+0x30);
 const float x=(m_.scalar(position)-minx)/(m_.scalar(info+0x34)-minx);
 const float y=(m_.scalar(position+4)-miny)/(m_.scalar(info+0x38)-miny);
 const auto ix=std::clamp(port::driving::cvt_w_s(x*8.f),0,7),iy=std::clamp(port::driving::cvt_w_s(y*8.f),0,7);
 if(x<=y){if(x<=1.f-y)return iy+16;return ix+24;}
 if(x<=1.f-y)return 16-ix;return 8-iy;
}
// Debug001B1BF0; E-0099-W22. Complete original record stays in mapped chunk.
int LoaderBodyDamage(Memory&m,std::uint32_t chunk){
 if(m.word(chunk)!=0x34030)return 0;
 constexpr std::uint32_t list=0x327fd8;const auto p=chunk+8,tail=m.word(list+4);
 m.word(tail,p);m.word(p+4,tail);m.word(list+4,p);m.word(p,list);
 m.word(p+0x4c,chunk+0x5c);m.word(p+0x50,0);return 1;
}
// Debug001B1C40; E-0099-W22.
int UnloaderBodyDamage(Memory&m,std::uint32_t chunk){
 if(m.word(chunk)!=0x34030)return 0;const auto p=chunk+8;
 m.word(m.word(p+4),m.word(p));m.word(m.word(p)+4,m.word(p+4));return 1;
}
// Debug001B20A0, E-0099-W22/W38. Embedded destructor now invokes the actual
// RepairAllDamage/ApplyDamage chain before unlinking, including live restoration.
void BodyDamage::Sub_001B20A0(){
 RepairAllDamage();
 const auto next=m_.word(address_),prev=m_.word(address_+4);
 m_.word(prev,next);m_.word(next+4,prev);
}
}
namespace port::race {
// Debug001B28D0 complete damage-list stores; E-0099-W22.
void initialize_body_damage_lists(port::driving::Memory&m){
 for(const auto list:{0x327fd8u,0x327fe0u}){m.word(list+4,list);m.word(list,list);}
}
}

#include "game_core/port/race/car_render_resources.hpp"
namespace hp2::debug {
// Debug001B21A0/001B1CB0; E-0099-W25. Real body model/vertex-count/info registration.
void BodyDamage::AddModel(std::uint32_t model,const char*name,int only){
 const auto count=m_.word(address_+0x4c);m_.word(address_+0x4c,count+1);const auto object=address_+0x50+28*count;
 m_.word(object+8,0);m_.word(object,address_);m_.word(object+16,static_cast<std::uint32_t>(only));m_.word(object+4,model);
 const auto solid=m_.word(model+12);const int vertices=solid?m_.signed_half(solid+0x38):0;m_.word(object+12,static_cast<std::uint32_t>(vertices));
 if(vertices){const auto info=FindBodyDamageInfo(m_,hp2::frontend::bStringHash(name));m_.word(object+8,info);if(info){if(vertices==m_.signed_word(info+0x44)){::port::race::CallDamageVertexFunction(m_,object);return;}m_.word(object+8,0);}}
}
}
namespace port::race {
// Debug001B1FE0; E-0099-W25. Actual body-model key traversal owner.
void CallDamageVertexFunction(Memory&m,std::uint32_t object){if(!m.word(object+8))return;m.word(object+20,0);m.word(object+24,0);CallDamageVertexFunctionForEveryVertex(m,m.word(object+4),object);}
// Debug001B1D60; E-0099-W25. Host vertex reference is the original callback's
// local XYZ carrier; unused vert_num/normal arguments have no recovered effect.
bool DamageVertexFunction(Memory&m,std::uint32_t object,std::array<float,3>&vertex,std::uint32_t rgba,bool damage_poly){
 const auto info=m.word(object+8),key=m.word(m.word(info+0x4c)+4*m.word(object+24));
 const auto repeated=m.word(object+20);if(repeated==(key&7)){m.word(object+20,0);m.word(object+24,m.word(object+24)+1);}else m.word(object+20,repeated+1);
 const auto sector=(key>>3)&31,body=m.word(object);const float applied=static_cast<float>(m.byte(body+0x2a+sector))*0.0078125f;
 const float amount=m.word(0x311fb8)?static_cast<float>(m.byte(body+8+sector))*0.0078125f:0.f;const float delta=amount-applied;
 if(delta!=0&&m.word(object+16)==0){
  if(key&0xffffc000u){const int shifts[]={12,6,0};for(unsigned axis=0;axis<3;++axis){const int signed_part=static_cast<std::int32_t>(key<<shifts[axis])>>26;vertex[axis]=vertex[axis]+(static_cast<float>(signed_part)*m.scalar(0x325858))*delta;}}
  const int darkness=(static_cast<std::int32_t>(key<<18)>>26)*8;
  if(darkness){const float f=static_cast<float>(darkness);const int adjustment=(driving::cvt_w_s(f*amount)>>3)-(driving::cvt_w_s(f*applied)>>3);if(adjustment)m.half(rgba,static_cast<std::uint16_t>(m.half(rgba)+adjustment*-1057));}
 }
 if(damage_poly){const int alpha=std::clamp(driving::cvt_w_s(amount*256.f),0,255);m.half(rgba,alpha<129?(m.half(rgba)&0x7fff):(m.half(rgba)|0x8000));}
 return amount!=0;
}
}

namespace hp2::debug {
// Debug001B2558, E-0099-W42. No clamp or invented damage amount.
float BodyDamage::GetVisibleTotalDamage()const{return m_.word(0x311fb8)?static_cast<float>(m_.half(address_+0x28))*0.000244140625f:0.f;}
}

#include <cmath>
namespace hp2::debug {
// Debug001B21D8, E-0099-W38. Exact clamp, CVT.W.S quantization, total halfword
// and replaced unsigned byte. Original consumers supply sector0..31.
void BodyDamage::SetDamage(int sector,float amount){if(sector<0||sector>=32)throw std::out_of_range("Original damage sector");if(amount>1)amount=1;if(amount<0)amount=0;if(!std::isfinite(amount))hp2::not_implemented("0x001B222C","Exceptional original damage CVT.W.S operand");const auto compressed=::port::driving::cvt_w_s(amount*128);const auto at=address_+8+static_cast<std::uint32_t>(sector);m_.half(address_+0x28,static_cast<std::uint16_t>(m_.half(address_+0x28)+compressed-m_.byte(at)));m_.byte(at,static_cast<std::uint8_t>(compressed));}
// Debug001B24F8/001B2538, E-0099-W38. Actual race visual-damage word, no clamp.
int BodyDamage::GetCompressedVisibleDamageAmount(int sector)const{if(sector<0||sector>=32)throw std::out_of_range("Original damage sector");return m_.word(0x311fb8)?m_.byte(address_+8+static_cast<std::uint32_t>(sector)):0;}
float BodyDamage::GetVisibleDamageAmount(int sector)const{return static_cast<float>(GetCompressedVisibleDamageAmount(sector))*0.0078125f;}
// Debug001B2598, E-0099-W38. Apply the existing original W25 raw model callback
// before updating applied sector bytes, preserving its read-before-write delta.
void BodyDamage::ApplyDamage(){const float visible=GetVisibleTotalDamage(),applied=static_cast<float>(m_.half(address_+0x4a))*0.000244140625f;if(visible==applied)return;
 if(std::fabs(GetVisibleTotalDamage()-applied)<=m_.scalar(0x32585c)&&GetVisibleTotalDamage()!=0)return;
 for(int i=0;i<m_.signed_word(address_+0x4c);++i)::port::race::CallDamageVertexFunction(m_,address_+0x50+static_cast<std::uint32_t>(i)*28);
 unsigned total=0;for(int i=0;i<32;++i){const auto value=GetCompressedVisibleDamageAmount(i);total+=static_cast<unsigned>(value);m_.byte(address_+0x2a+static_cast<std::uint32_t>(i),static_cast<std::uint8_t>(value));}m_.half(address_+0x4a,static_cast<std::uint16_t>(total));
}
// Debug001B2250, E-0099-W38. Restore actual original model vertices/colour/flags
// through ApplyDamage, then source destructor unlinks the containing node.
void BodyDamage::RepairAllDamage(){for(int sector=0;sector<32;++sector)SetDamage(sector,0);m_.half(address_+0x28,0);ApplyDamage();}
}

namespace hp2::debug {
// Debug001B26E0, E-0099-W38. Apply to every actual registered model, in order.
void BodyDamage::SetEnvironmentMapping(std::uint32_t hash,int on){for(int i=0;i<m_.signed_word(address_+0x4c);++i)::port::race::eModel(m_,m_.word(address_+0x54+static_cast<unsigned>(i)*28)).SetEnvironmentMapping(hash,on);}
}

namespace hp2::debug {
// Debug001B2760, E-0099-W67. Original JR RA/NOP, no state read or write.
void ServiceTweakerDamage(){}
}

#include "game_core/indep/inc/Replay.hpp"
namespace hp2::debug {
// Debug001B20E8; TASK-0099 E-0099-W74; replay-frame/001b20e8.json/type-BodyDamage.json.
void BodyDamage::DoSnapshot(std::uint32_t snapshot){
 ReplaySnapshot s(m_,snapshot);s.BeginSection(0x2ae368);s.MoveData(address_+8,32,32,0);
 s.MoveData(address_+0x28,2);if(m_.byte(snapshot+0x3d))ApplyDamage();s.EndSection();
}
}
