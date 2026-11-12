// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/src/joystick.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x00153C20; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00153c20.md.
void Joystick::FindEventNode() {
    hp2::not_implemented("0x00153C20", "Joystick::FindEventNode (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x00153A00; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00153a00.md.
void QueuedJoystickEvents::AddEvent() {
    hp2::not_implemented("0x00153A00", "QueuedJoystickEvents::AddEvent (signature and full body unresolved)");
}

// ELF 0x00153A50; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00153a50.md.
void QueuedJoystickEvents::RemoveEvent() {
    hp2::not_implemented("0x00153A50", "QueuedJoystickEvents::RemoveEvent (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/port/race/physics_construction.hpp"
namespace hp2::debug {
// Debug00153F20, E-0099-W42. This is the real current input-type table; device
// polling/publication is a separate original/platform dependency.
std::uint8_t GetJoystickType(::port::driving::Memory& m,int port){return m.byte(0x3273b8+static_cast<std::uint32_t>(port));}
}

#include "game_core/indep/src/table.hpp"
#include "game_core/inc/bSlotPool.hpp"
namespace hp2::debug {
// E-0099-W62, original00155228. Two joystick list/calibration components only;
// PortNumber and all unmentioned bytes retain the original preexisting state.
void Sub_00155228(::port::driving::Memory& m){
 for(unsigned i=0;i<2;++i){const auto a=0x2f0da8u+204*i;m.word(a,a);m.word(a+4,a);
  for(unsigned j=0;j<4;++j){const auto p=a+8+44*j;hp2::recovered::Graph(m,p+32).Sub_0018D9E8(p,4);for(unsigned k=0;k<4;++k){const float value=k==0?0.f:(k==3?255.f:128.f);m.scalar(p+8*k,value);m.scalar(p+8*k+4,value);}m.byte(p+40,0);}
  m.word(a+200,0);m.word(a+188,0xffffffff);m.word(a+192,0xffffffff);m.word(a+196,0);
 }m.word(0x2f0f40,0);
}
std::uint32_t Joystick::FindEventNode(int event)const{auto node=m_.word(address_);if(!node)hp2::not_implemented("0x00153870","original Joystick list static initialization is required");for(;node!=address_;node=m_.word(node))if(m_.word(node+8)==static_cast<std::uint32_t>(event))return node;return 0;}
void QueuedJoystickEvents::RemoveEvent(int port,int event){for(int i=0;i<m_.signed_word(address_);++i){const auto row=address_+4+12*i;if(m_.byte(row+2)==static_cast<std::uint32_t>(port)&&m_.byte(row+3)==(static_cast<unsigned>(event)&255))m_.word(row+4,0);}}
int IsJoyHandlerAdded(::port::driving::Memory& m,int port,int event){if(port<0||port>=2)hp2::not_implemented("0x00153EC8","Joystick port is outside original two-object array");return Joystick(m,0x2f0da8+204*port).FindEventNode(event)!=0;}
void RemoveJoyHandler(::port::driving::Memory& m,int port,int event){QueuedJoystickEvents(m,0x2f0f40).RemoveEvent(port,event);if(port<0||port>=2)hp2::not_implemented("0x00153E38","Joystick port is outside original two-object array");const auto node=Joystick(m,0x2f0da8+204*port).FindEventNode(event);if(node){const auto next=m.word(node),prev=m.word(node+4);m.word(prev,next);m.word(next+4,prev);SlotPool(m,m.word(0x3273a8)).Free(node);}}
}

#include "game_core/indep/inc/Replay.hpp"
namespace hp2::debug {
// Debug001540B0; E-0099-W74; replay-frame/001540b0.json/time-joy-symbols.json.
void JoystickDoSnapshot(::port::driving::Memory& m,std::uint32_t snapshot){ReplaySnapshot s(m,snapshot);s.BeginSection(0x2a8178);s.MoveData(0x3273b8,2);s.EndSection();}
}

namespace hp2::debug {
// E-0099-W77; Debug00154BF8,00154C88,00154CF0. Source signed configuration
// bytes, bounded first matching type and original table-order selection.
int DetermineAvailableJoystickConfigurations(::port::driving::Memory& m,std::uint32_t output,int maximum,int type){
 for(int i=0;i<m.signed_word(0x326c44);++i){const auto row=0x2d5e78u+20u*i;if(m.signed_word(row)!=type)continue;int count=0;for(unsigned j=0;j<10;++j){const int value=m.signed_byte(row+4+j);if(value&&count<maximum){m.word(output+4*count,std::uint32_t(value));++count;}}return count;}return 0;
}
int ChooseJoystickConfigurationNumber(::port::driving::Memory& m,int type,int selected){
 ::port::driving::Memory::Temporary local(m,40);const int count=DetermineAvailableJoystickConfigurations(m,local.address(),10,type);if(!count)return 0;for(int i=0;i<count;++i)if(m.signed_word(local.address()+4*i)==selected)return selected;return m.signed_word(local.address());
}
std::uint32_t FindScannerConfig(::port::driving::Memory& m,int event,int type,int selected){
 const int configuration=ChooseJoystickConfigurationNumber(m,type,selected);for(int i=0;i<m.signed_word(0x326c40);++i){const auto row=0x2d3ac0u+36u*i;if(m.signed_word(row+8)!=event)continue;for(unsigned j=0;j<8;++j){if(!configuration){if(event==1)return row;}else{const int value=m.signed_byte(row+j);if(value&&(value==-1||value==configuration))return row;}}}return 0;
}
// Debug001538A0; source config/first-event state, no hardware sampling.
void Joystick::ConnectEventNodeConfig(std::uint32_t node){m_.word(node+0xc,FindScannerConfig(m_,m_.signed_word(node+8),m_.signed_word(address_+188),m_.signed_word(address_+200)));m_.word(node+0x1c,1);m_.word(node+0x18,0);}
// Debug00153D90;00153E30 is instruction-proven alternate-signature thunk.
// The original duplicate lookup result is intentionally not used.
void AddJoyHandler(::port::driving::Memory& m,int port,int event,std::uint32_t handler,std::uint32_t object){
 if(port<0||port>=2)
  hp2::not_implemented("0x00153D90","Joystick port is outside original two-object array");
 const auto address=0x2f0da8u+204u*port;
 Joystick joystick(m,address);
 joystick.FindEventNode(event);
 const auto pool=m.word(0x3273a8);
 if(!pool)
  hp2::not_implemented("0x00153D90","original InitJoystick EventNodeSlotPool prefix is required");
 const auto node=bOMalloc(m,pool),prev=m.word(address+4);
 m.word(node+8,std::uint32_t(event));
 m.word(prev,node);m.word(address+4,node);
 m.word(node+0x10,handler);m.word(node+0x14,object);
 m.word(node,address);m.word(node+4,prev);
 m.word(node+0xc,0);m.word(node+0x1c,0);m.word(node+0x18,0);m.word(node+0x20,0);
 joystick.ConnectEventNodeConfig(node);
}
}
