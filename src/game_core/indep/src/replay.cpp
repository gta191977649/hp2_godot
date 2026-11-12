// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\replay.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): ReplayManager::BeginFrame() [0x002AEF30]
// Diagnostic text (not a callable signature): ReplayManager::HandleSnapshot() [0x002AEFF8]
#include "game_core/ReplayManager.hpp"
#include <string>

namespace hp2::recovered {
// Debug0013E0F0, E-0099-W122; original replay.cpp. Existing registration owns
// event scoring/storage; this wrapper does not replace the source replay owner.
void ReplayManager::InvalidateInterestingEvents(){RegisterInterestingEvent(-99.f,"Invalidate");}
// Debug0013E070; E-0046-C01. Name is intentionally unused by the original.
void ReplayManager::RegisterInterestingEvent(float value,const char*){
    if(m_.word(address_+4)==0)return;
    const auto head=address_+0x6c;
    for(auto node=m_.word(head);node!=head;node=m_.word(node)){
        const char expected[]="INSTANT_REPLAY";unsigned i=0;
        while(expected[i]!=0&&m_.byte(node+8+i)==static_cast<unsigned char>(expected[i]))++i;
        if(expected[i]==0&&m_.byte(node+8+i)==0)m_.scalar(node+0x14c,m_.scalar(node+0x14c)+value);
    }
}
}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/inc/Replay.hpp"

#include "game_core/port/not_implemented.hpp"
#include "game_core/indep/src/brepeat.hpp"

namespace hp2::recovered {
// ELF 0x0013CD68; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0013cd68.md.
void ReplaySnapshot::GetChecksum() {
    hp2::not_implemented("0x0013CD68", "ReplaySnapshot::GetChecksum (signature and full body unresolved)");
}

// ELF 0x0013CE78; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0013ce78.md.
void ReplaySnapshot::ReadData() {
    hp2::not_implemented("0x0013CE78", "ReplaySnapshot::ReadData (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/port/not_implemented.hpp"
#include "game_core/indep/src/brepeat.hpp"
namespace hp2::recovered {
// Debug0013D248, E-0099-W41. Do not clear the omitted MaxNumAutoReplaySnapshots,
// FancyJoyEventTable or SnapshotTimeTable; source leaves those bytes untouched.
void ReplayManager::Sub_0013D248(){
 auto& m=m_;const auto a=address_,free=a+0x64,used=a+0x6c;
 m.word(free,free);m.word(free+4,free);m.word(used,used);m.word(used+4,used);
 m.word(a+0xa0,0xffffffffu);m.scalar(a+0x94,15);m.word(a+0x18,9999999);
 m.word(a+0x4c,0xffffffffu);m.word(a+0x50,0xffffffffu);
 for(auto off:{0u,4u,8u,0xcu,0x10u,0x14u,0x54u,0x58u,0x5cu,0x48u,0xa4u,0xa8u,0x1cu,0x20u,0x24u,0x60u,0x74u,0x78u,0x80u,0x84u,0x88u,0x8cu,0x90u,0x98u,0x9cu,0x40acu,0x40b0u})m.word(a+off,0);
 for(unsigned i=0;i<0x1000;++i)m.byte(a+0x40b4+i,0);
 if(m.word(0x2e81f0)){
  m.word(a+0x60,m.word(0x2e8224));m.word(0x2e8224,0);
  // Original AddTail0013EA80 moves a complete intrusive list and resets source.
  const auto source=0x2e8228u,first=m.word(source),last=m.word(source+4);
  if(first!=source){const auto previous=m.word(used+4);m.word(last,used);m.word(first+4,previous);m.word(previous,first);m.word(used+4,last);m.word(source+4,source);m.word(source,source);}
  m.word(a+0x74,1);
  int i=0;do{const auto snapshot=m.allocate(0x9558);m.word(snapshot+0x28,0);m.word(snapshot+0x2c,0);
   const auto previous=m.word(free+4);m.word(previous,snapshot);m.word(free+4,snapshot);m.word(snapshot+4,previous);m.word(snapshot,free);++i;
  }while(i<m.signed_word(a+0x74));
 }
}
// Debug0013D3C0, E-0099-W41. Snapshot dtor0013EB40 only releases on bit1.
void ReplayManager::Sub_0013D3C0(int flags){
 auto& m=m_;const auto a=address_,used=a+0x6c,free=a+0x64;
 while(m.word(used)!=used){const auto snapshot=m.word(used),next=m.word(snapshot),previous=m.word(snapshot+4);const auto image=m.word(snapshot+0x38);
  m.word(previous,next);m.word(next+4,previous);
  if(!image)m.release(snapshot);
  else {const auto repeat=m.word(a+0x60),demo=0x2e8228u,tail=m.word(demo+4);m.word(tail,snapshot);m.word(snapshot+4,tail);m.word(demo+4,snapshot);m.word(snapshot,demo);if(repeat){m.word(0x2e8224,repeat);m.word(a+0x60,0);}}
 }
 while(m.word(free)!=free){const auto snapshot=m.word(free),next=m.word(snapshot),previous=m.word(snapshot+4);m.word(previous,next);m.word(next+4,previous);m.release(snapshot);}
 if(const auto repeat=m.word(a+0x60))m.release(repeat);
 if(flags&1)m.release(a);
}
// Debug0013E978/0013EA10, E-0099-W41. Inactive original branches perform no
// repeat access; active bitstream behavior is a required dependency, not zero data.
std::uint32_t ReplayManager::GetData(int bits,int channel){
 if(!m_.word(address_)||m_.word(address_+8))return 0;
 const auto repeat=m_.word(address_+0x60);const auto data=hp2::debug::bRepeat(m_,repeat).RemoveData(bits,channel);
 if(m_.word(repeat+0x28))m_.word(address_+8,1);return static_cast<std::uint32_t>(data);
}
void ReplayManager::MarkTheEndOfTime(){if(!m_.word(address_+0x54))m_.word(address_+0x54,1);}
void ReplayManager::AddData(std::int32_t data,int bits,int channel){
 if(!m_.word(address_+4))return;const auto repeat=m_.word(address_+0x60);hp2::debug::bRepeat(m_,repeat).AddData(data,bits,channel);
 if(static_cast<std::int32_t>(m_.word(repeat+0x14)*8-0x44)<=m_.signed_word(repeat+8)){m_.word(address_+4,0);MarkTheEndOfTime();}
}
}

#include "game_core/port/race/replay_memory.hpp"
namespace hp2::recovered {
// Debug0013E9D8, E-0099-W41. Fresh MIPS0013EA00 is SRLV, not SRAV.
// Preserve the misleading original name: values below32 bits are zero-extended.
std::int32_t ReplayManager::GetSignedData(int bits,int channel){const auto value=GetData(bits,channel);const unsigned shift=(32u-static_cast<unsigned>(bits))&31;return static_cast<std::int32_t>((value<<shift)>>shift);}
// Debug0013D4F8, E-0099-W41. Actual free-heap query is a required platform
// primitive; no forced sufficient-memory result, skipped snapshot pool or flag.
void ReplayManager::MarkTheBeginningOfTime(const ::port::race::ReplayMemoryServices& services){
 auto& m=m_;const auto a=address_;if(m.signed_word(a+0x4c)!=-1)return;
 m.word(a+0x50,m.word(0x327d10));m.word(a+0x4c,m.word(0x327d10));m.word(a+0x58,m.word(0x327cf8));m.word(a+0x5c,m.word(0x327cf8));
 if(!services.count_free_memory)hp2::not_implemented("0x00250D58","original replay free-heap primitive needs its actual port allocation budget binding");
 const auto free=services.count_free_memory(0x10);if(static_cast<std::int32_t>(static_cast<std::uint32_t>(free)-0x8000u)<0xd0ad8)return;
 const auto repeat=m.allocate(0x32000);hp2::debug::bRepeat(m,repeat).PrepareToAddData(0x32000,4,0);
 m.word(repeat+0x54,4);m.word(repeat+0x30,8);m.word(repeat+0x58,1);m.word(repeat+0x34,0);m.word(repeat+0x78,0);m.word(repeat+0x7c,0);m.word(repeat+0x9c,8);m.word(repeat+0xa0,0);
 m.word(a+0x60,repeat);m.word(a+0x74,17);m.word(a+0x78,2);m.word(a+0x80,8);m.word(a+0x84,5);m.word(a+0x54,0);m.word(a+0x7c,0);
 int i=0;const auto head=a+0x64;do{const auto snapshot=m.allocate(0x9558);m.word(snapshot+0x28,0);m.word(snapshot+0x2c,0);const auto previous=m.word(head+4);m.word(previous,snapshot);m.word(head+4,snapshot);m.word(snapshot+4,previous);m.word(snapshot,head);++i;}while(i<m.signed_word(a+0x74));
}
}

// TASK-0099 E-0099-W74. Debug NFSHP2.ELF SHA-256
// 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
// Raw read-only queries: local/task0099/replay-frame/0013{cae8,cb68,cba0,cc78,cd08,ced0,cef8,cf00,d088,d158}.json.
namespace hp2::debug {
// Debug0013CAE8. Init does not reset the reused snapshot's EndTime+2C.
void ReplaySnapshot::Init(std::uint32_t name){
 auto& m=m_;const auto a=address_;unsigned i=0;
 do{const auto c=m.byte(name+i);m.byte(a+8+i,c);++i;if(!c)break;
  if(i>=32)hp2::not_implemented("0x0013CAE8","Snapshot name exceeds original inline storage");
 }while(true);
 m.word(a+0x28,m.word(0x327cf8));m.word(a+0x30,m.word(0x327d10));
 m.byte(a+0x40,255);m.word(a+0x44,0xffffffffu);
 for(auto off:{0x154u,0x34u,0x38u,0x3cu,0x48u,0x14cu,0x150u})m.word(a+off,0);
 m.byte(a+0x41,0);m.half(a+0x42,0);
}
// Debug0013CB68. memory_size is unused by the original hash.
std::uint32_t ReplaySnapshot::GetChecksum(std::uint32_t pdata,int data_size,int)const{
 std::uint32_t result=0xffffffffu;
 for(int i=0;i<data_size;++i)result=result*33u+m_.byte(pdata+static_cast<unsigned>(i));
 return result;
}
// Debug0013CBA0. Capacity test is strictly less-than; overflow does not advance.
void ReplaySnapshot::WriteData(std::uint32_t pdata,int size,int memory_size,int sign_extend){
 auto& m=m_;const auto a=address_;auto pos=m.word(a+0x150);
 if(size<0||memory_size<0||pos>0x9400u)hp2::not_implemented("0x0013CBA0","Invalid snapshot byte-count domain");
 if(static_cast<std::uint64_t>(pos)+static_cast<unsigned>(size)<0x9400u){
  std::uint8_t last=0;for(int i=0;i<size;++i){last=m.byte(pdata+i);m.byte(a+0x158+pos++,last);}
  const auto extension=static_cast<std::uint8_t>(sign_extend&&(last&128)?255:0);
  for(int i=size;i<memory_size;++i)if(m.byte(pdata+i)!=extension)
   hp2::not_implemented("0x0013CBA0",(std::string("Original bBreak: snapshot field does not fit serialized width; address=")+std::to_string(pdata)+" dataSize="+std::to_string(size)+" memorySize="+std::to_string(memory_size)+" tailOffset="+std::to_string(i)).c_str());
 }else if(!m.byte(a+0x41))m.byte(a+0x41,1);
 m.word(a+0x150,pos);
}
// Debug0013CC78. A short input genuinely leaves destination and cursor unchanged.
void ReplaySnapshot::ReadData(std::uint32_t pdata,int size,int memory_size,int sign_extend){
 auto& m=m_;const auto a=address_;auto pos=m.word(a+0x150);
 if(size<0||memory_size<0)hp2::not_implemented("0x0013CC78","Invalid snapshot byte-count domain");
 if(static_cast<std::uint64_t>(pos)+static_cast<unsigned>(size)<=m.word(a+0x154)){
  std::uint8_t last=0;for(int i=0;i<size;++i){last=m.byte(a+0x158+pos++);m.byte(pdata+i,last);}
  const auto extension=static_cast<std::uint8_t>(sign_extend&&(last&128)?255:0);
  for(int i=size;i<memory_size;++i)m.byte(pdata+i,extension);
 }
 m.word(a+0x150,pos);
}
// Debug0013CD08. Capture/restore/checksum/verify priority and per-section byte counter.
void ReplaySnapshot::MoveData(std::uint32_t pdata,int size,int memory_size,int sign_extend){
 auto& m=m_;const auto a=address_;const int depth=m.signed_byte(a+0x40);
 if(depth<0||depth>=16)hp2::not_implemented("0x0013CD08","MoveData outside original section stack");
 const auto section=a+0x4c+16u*depth;m.byte(section+10,m.byte(section+10)+1);
 const int extent=memory_size==-1?size:memory_size;
 if(m.byte(a+0x3c))WriteData(pdata,size,extent,sign_extend);
 else if(m.byte(a+0x3d))ReadData(pdata,size,extent,sign_extend);
 else if(m.byte(a+0x3e)||m.byte(a+0x3f)){
  const auto checksum=static_cast<std::uint16_t>(GetChecksum(pdata,size,extent));
  ::port::driving::Memory::Temporary local(m,2);m.half(local.address(),checksum);
  if(m.byte(a+0x3e))WriteData(local.address(),2,2,0);
  else{
   if(static_cast<std::uint64_t>(m.word(a+0x150))+2>m.word(a+0x154))
    hp2::not_implemented("0x0013CD08","Short checksum input leaves original prior-stack halfword unresolved");
   ReadData(local.address(),2,2,0);
   if(checksum!=m.half(local.address())&&!m.byte(section+11)){
    m.byte(section+11,1);
    hp2::not_implemented("0x0013CD08","Original checksum diagnostic/bBreak after snapshot mismatch");
   }
  }
 }
}
// Debug0013CED0. VerifyData is intentionally inactive during ordinary take/restore.
void ReplaySnapshot::VerifyData(std::uint32_t p,int size,int memory_size,int sign_extend){
 if(m_.byte(address_+0x3e)||m_.byte(address_+0x3f))MoveData(p,size,memory_size,sign_extend);
}
// Debug0013CEF8. Exact empty source body, not a substitute synchronization marker.
void ReplaySnapshot::VerifyMoveDataInSync(std::uint32_t,std::uint32_t){}
// Debug0013D088. Raw name/object addresses remain borrowed until EndSection.
void ReplaySnapshot::BeginSection(std::uint32_t object,std::uint32_t name){
 auto& m=m_;const auto a=address_;const int old=m.signed_byte(a+0x40),depth=old+1;
 if(depth<0||depth>=16)hp2::not_implemented("0x0013D088","Original snapshot section stack overflow");
 if(old==-1)m.half(a+0x42,0);
 const auto number=m.half(a+0x42);m.byte(a+0x40,depth);m.half(a+0x42,number+1);
 const auto s=a+0x4c+16u*depth;m.byte(s+10,0);m.byte(s+11,0);m.half(s+8,number);
 m.word(s+4,object);m.word(s+12,m.word(a+0x150));m.word(s,name);
 if(m.signed_word(a+0x44)>=0&&m.signed_word(a+0x44)<=depth&&m.word(a+0x48)==m.word(0x327d10))
  hp2::not_implemented("0x0013D088","Active PrintSectionName/PrintSpaces diagnostic output");
}
// Debug0013D158.
void ReplaySnapshot::EndSection(){
 auto& m=m_;const auto a=address_;const int depth=m.signed_byte(a+0x40);
 if(depth<0||depth>=16)hp2::not_implemented("0x0013D158","EndSection outside original section stack");
 const auto s=a+0x4c+16u*depth;m.word(s+12,m.word(a+0x150)-m.word(s+12));
 m.byte(a+0x40,depth-1);if(depth-1<m.signed_word(a+0x44))m.word(a+0x44,0xffffffffu);
}
// Debug0013CF00. Compact affine matrix format, including source capture-side W writes.
void ReplaySnapshot::MoveData(::port::race::SnapshotMatrix matrix,int){
 auto& m=m_;const auto p=matrix.address;
 // EE comparisons of denormal inputs have not been established for this overload.
 for(unsigned off:{12u,28u,44u,60u,48u,52u,56u}){
  const auto bits=m.word(p+off);
  if((bits&0x7f800000u)==0&&(bits&0x7fffffu)!=0)
   hp2::not_implemented("0x0013CF00","EE denormal matrix classification comparison");
 }
 std::uint8_t type=0;
 if(m.scalar(p+12)==0&&m.scalar(p+28)==0&&m.scalar(p+44)==0&&m.scalar(p+60)==1){
  type=2;if(m.scalar(p+48)==0&&m.scalar(p+52)==0&&m.scalar(p+56)==0)type=1;
 }
 ::port::driving::Memory::Temporary local(m,1);m.byte(local.address(),type);MoveData(local.address(),1);type=m.byte(local.address());
 if(type==0){MoveData(p,64);return;}
 for(unsigned off:{0u,16u,32u})MoveData(p+off,12);
 for(unsigned off:{12u,28u,44u})m.word(p+off,0);
 m.scalar(p+60,1);
 if(type!=2){m.word(p+48,0);m.word(p+52,0);m.word(p+56,0);return;}
 MoveData(p+48,12);
}
}

#include "game_core/indep/inc/Replay.hpp"
namespace hp2::debug {
// Debug0013C990; E-0099-W74; replay-frame/0013c990.json/type-bRepeatChannel.json.
void bRepeat::DoSnapshot(std::uint32_t snapshot,bool add_data_position){
 ReplaySnapshot s(m_,snapshot);s.BeginNamedSection(0x3271b0);const auto a=address_;
 if(!add_data_position)m_.word(a+0x28,0);
 s.MoveData(a+(add_data_position?8:0x10),4,4,1);s.MoveData(a+(add_data_position?0x20:0x24),4,4,1);
 for(int i=0;i<m_.signed_word(a+0x18);++i){const auto c=a+0x30+0x24*i+(add_data_position?12:24);s.MoveData(c,4,4,1);s.MoveData(c+4,4,4,1);s.MoveData(c+8,4,4,1);}
 s.EndNamedSection();
}
}

#include "game_core/indep/inc/RaceCoordinator.hpp"
#include "game_core/indep/src/joystick.hpp"
#include "game_core/indep/src/raceparameters.hpp"
#include "game_core/port/driving/projections.hpp"
#include <cstring>
#include <cstdio>
namespace hp2::recovered {
// Debug0013D7B8; E-0099-W74; replay-frame/0013d7b8.json.
bool ReplayManager::WasSnapshotTakenAtThisTime(int time){
 int index=0;const auto last=m_.signed_word(address_+0xa8);
 if(last<0||last>=4096)hp2::not_implemented("0x0013D7B8","Snapshot history cursor outside source array");
 if(last&&m_.signed_word(address_+0xac+4*last)<time)index=last;
 for(;index<m_.signed_word(address_+0xa4);++index){if(index>=4096)hp2::not_implemented("0x0013D7B8","Snapshot history count outside source array");if(m_.signed_word(address_+0xac+4*index)==time){m_.word(address_+0xa8,index);return true;}}
 return false;
}
// Debug0013E3B8; E-0099-W74; exact case-sensitive source name/order/countdown.
std::uint32_t ReplayManager::FindSnapshot(const char* name,int number){
 const auto head=address_+0x6c;for(auto p=m_.word(head);p!=head;p=m_.word(p)){
  unsigned i=0;while(name[i]&&m_.byte(p+8+i)==static_cast<unsigned char>(name[i]))++i;
  if(!name[i]&&!m_.byte(p+8+i)){if(number==0)return p;number=static_cast<int>(static_cast<std::uint32_t>(number)-1);}
 }return 0;
}
// Debug0013E5C8; E-0099-W74. Move used node to free tail, never release here.
void ReplayManager::DeleteSnapshot(std::uint32_t p){const auto next=m_.word(p),previous=m_.word(p+4),head=address_+0x64,tail=m_.word(head+4);m_.word(previous,next);m_.word(next+4,previous);m_.word(tail,p);m_.word(head+4,p);m_.word(p,head);m_.word(p+4,tail);}
// Debug0013E580; E-0099-W74.
int ReplayManager::DeleteSnapshot(const char* name){const auto p=FindSnapshot(name,0);if(p)DeleteSnapshot(p);return p!=0;}
// Debug0013E6C8; E-0099-W74; replay-frame/0013e6c8.json.
void ReplayManager::DoEndSnapshot(std::uint32_t snapshot){
 hp2::debug::ReplaySnapshot s(m_,snapshot);s.BeginSection(0x2a7060);
 for(unsigned off:{0x4cu,0x50u,0xa4u})s.MoveData(address_+off,4,4,1);
 for(int i=0;i<m_.signed_word(address_+0xa4);++i){if(i>=4096)hp2::not_implemented("0x0013E6C8","Snapshot time count exceeds source table");s.MoveData(address_+0xac+4*i,4,4,1);}
 s.MoveData(address_+0x40ac,4,4,1);
 for(int i=0;i<m_.signed_word(address_+0x40ac);++i){if(i>=256)hp2::not_implemented("0x0013E6C8","Special event count exceeds source table");s.MoveData(address_+0x40b4+16*i,16);}
 s.EndSection();
}
// Debug0013E5F8; E-0099-W74; no synthetic top-level section is inserted.
void ReplayManager::HandleSnapshot(std::uint32_t snapshot,const ::port::race::SnapshotServices& services){
 const char expected[]="REPLAY_MANAGER_STATE";unsigned i=0;while(expected[i]&&m_.byte(snapshot+8+i)==static_cast<unsigned char>(expected[i]))++i;
 if(!expected[i]&&!m_.byte(snapshot+8+i)){DoEndSnapshot(snapshot);return;}
 hp2::debug::RaceParameters(m_,0x311f78).DoSnapshot(snapshot);
 const bool add=(m_.byte(snapshot+0x3c)||m_.byte(snapshot+0x3e))&&m_.word(0x327d10)==m_.word(address_+0x50);
 hp2::debug::bRepeat(m_,m_.word(address_+0x60)).DoSnapshot(snapshot,add);
 hp2::debug::JoystickDoSnapshot(m_,snapshot);
 hp2::debug::RaceCoordinator(m_,m_.word(0x327290)).DoSnapshot(snapshot,services);
}
// Debug0013E7E0; E-0099-W74. Name is a host string operand copied into original inline storage.
std::uint32_t ReplayManager::TakeSnapshot(const char* name,const ::port::race::SnapshotServices& services){
 auto& m=m_;const auto a=address_,free=a+0x64,used=a+0x6c;if(m.signed_word(a+0x4c)<0)return 0;
 auto p=m.word(free);if(p==free){p=m.allocate(0x9558);m.word(p+0x28,0);m.word(p+0x2c,0);const auto tail=m.word(free+4);m.word(tail,p);m.word(free+4,p);m.word(p,free);m.word(p+4,tail);p=m.word(free);}
 const auto next=m.word(p),previous=m.word(p+4);m.word(previous,next);m.word(next+4,previous);const auto tail=m.word(used+4);m.word(tail,p);m.word(used+4,p);m.word(p+4,tail);m.word(p,used);
 ::port::driving::Memory::Temporary text(m,static_cast<std::uint32_t>(std::strlen(name)+1));for(unsigned j=0;j<=std::strlen(name);++j)m.byte(text.address()+j,static_cast<unsigned char>(name[j]));hp2::debug::ReplaySnapshot(m,p).Init(text.address());
 const auto loop=m.word(0x327d10);if(loop==m.word(a+0x50)&&m.signed_word(a+0xa4)<4096){const auto count=m.word(a+0xa4);m.word(a+0xac+4*count,loop);m.word(a+0xa4,count+1);}m.word(a+0xa0,loop);
 m.word(p+0x150,0);m.byte(p+0x3c,1);HandleSnapshot(p,services);m.word(p+0x154,m.word(p+0x150));m.byte(p+0x3c,0);if(m.byte(p+0x41)){DeleteSnapshot(p);return 0;}return p;
}
// Debug0013D820; E-0099-W74.
void ReplayManager::MaybeTakeDummySnapshot(const ::port::race::SnapshotServices& services){
 auto& m=m_;const auto a=address_;const auto loop=m.signed_word(0x327d10);bool take=false;
 if(m.word(a)&&!m.word(a+0x14)&&m.signed_word(a+0xa0)!=loop)take=WasSnapshotTakenAtThisTime(loop);
 if(m.word(a+4)&&(loop%211==0||static_cast<std::int32_t>(m.word(a+0xa0)+30)<loop))take=true;
 if(take&&TakeSnapshot("DUMMY_SNAPSHOT",services))DeleteSnapshot("DUMMY_SNAPSHOT");
}
}

namespace hp2::recovered {
// Debug0013D8F0; E-0099-W74; replay-frame/0013d8f0.json.
void ReplayManager::MaybeTakeInstantReplaySnapshot(const ::port::race::SnapshotServices& services){
 auto& m=m_;const auto a=address_;if(m.signed_word(0x327d04)<m.signed_word(a+0x48))return;
 m.word(a+0x48,m.word(0x327d04)+300);
 if(m.signed_word(a+0x88)!=0&&m.signed_word(a+0x88)>=m.signed_word(a+0x78)){
  const auto instant=FindSnapshot("INSTANT_REPLAY",0);if(!instant)hp2::not_implemented("0x0013D8F0","Original instant count has no matching snapshot node");
  std::uint32_t minimum=0,candidate=0;bool overlap=false;
  for(int i=0;i<m.signed_word(a+0x84);++i){if(i<m.signed_word(a+0x90)){
   const auto p=FindSnapshot("HIGHLIGHT_SNAPSHOT",i);if(!p)hp2::not_implemented("0x0013D8F0","Original highlight count has no matching snapshot node");
   if(m.signed_word(instant+0x30)<m.signed_word(p+0x34)){candidate=p;overlap=true;break;}
   if(!minimum||m.scalar(p+0x14c)<m.scalar(minimum+0x14c))minimum=p;
  }else minimum=0;}
  if(!overlap)candidate=minimum;
  const auto interesting=m.scalar(instant+0x14c);
  if(0<interesting&&(!candidate||interesting>m.scalar(candidate+0x14c))){
   if(m.word(a+0x90)==m.word(a+0x84)||(candidate&&m.signed_word(instant+0x30)<m.signed_word(candidate+0x34))){
    if(!candidate)hp2::not_implemented("0x0013D8F0","Original highlight replacement would dereference null snapshot");
    DeleteSnapshot(candidate);m.word(a+0x90,m.word(a+0x90)-1);
   }
   const char text[]="HIGHLIGHT_SNAPSHOT";for(unsigned i=0;i<sizeof(text);++i)m.byte(instant+8+i,static_cast<unsigned char>(text[i]));
   m.word(instant+0x34,m.word(0x327d10));m.word(instant+0x2c,m.word(0x327cf8));m.word(a+0x88,m.word(a+0x88)-1);m.word(a+0x90,m.word(a+0x90)+1);
  }else{DeleteSnapshot("INSTANT_REPLAY");m.word(a+0x88,m.word(a+0x88)-1);}
 }
 if(TakeSnapshot("INSTANT_REPLAY",services))m.word(a+0x88,m.word(a+0x88)+1);
}
// Debug0013DCD8; E-0099-W74. Original thinning removes odd-indexed source nodes as the list shrinks.
void ReplayManager::MaybeTakeLapSnapshot(const ::port::race::SnapshotServices& services){
 auto& m=m_;const auto a=address_;
 for(;;){const float elapsed=static_cast<float>(static_cast<std::int32_t>(m.word(0x327cf8)-m.word(a+0x58)))*m.scalar(0x324ecc);
  if(elapsed<m.scalar(a+0x94)*static_cast<float>(m.signed_word(a+0x8c)))return;
  if(m.word(a+0x8c)!=m.word(a+0x80))break;
  m.scalar(a+0x94,m.scalar(a+0x94)+m.scalar(a+0x94));int index=0;
  while(FindSnapshot("LAP_SNAPSHOT",index)){++index;const auto p=FindSnapshot("LAP_SNAPSHOT",index);if(p){DeleteSnapshot(p);m.word(a+0x8c,m.word(a+0x8c)-1);}}
  if(m.signed_word(a+0x80)<=m.signed_word(a+0x8c))return;
 }
 if(TakeSnapshot("LAP_SNAPSHOT",services))m.word(a+0x8c,m.word(a+0x8c)+1);
}
// Debug0013DB88; E-0099-W74; checksum snapshots do not enter SnapshotTimeTable.
void ReplayManager::DoChecksumSnapshot(const char* name,const ::port::race::SnapshotServices& services){
 auto& m=m_;const auto a=address_;if(!m.word(a+4)){
  if(m.word(a)&&!m.word(a+0x14)){if(const auto p=FindSnapshot(name,0)){m.word(p+0x150,0);m.byte(p+0x3f,1);HandleSnapshot(p,services);m.byte(p+0x3f,0);}}return;
 }
 if(FindSnapshot(name,0))hp2::not_implemented("0x0013DB88","Original bBreak on duplicate named checksum snapshot");
 const auto free=a+0x64,used=a+0x6c;auto p=m.word(free);
 if(p==free){p=m.allocate(0x9558);m.word(p+0x28,0);m.word(p+0x2c,0);const auto tail=m.word(free+4);m.word(tail,p);m.word(free+4,p);m.word(p+4,tail);m.word(p,free);p=m.word(free);}
 if(p!=free){const auto next=m.word(p),previous=m.word(p+4),tail=m.word(used+4);m.word(previous,next);m.word(next+4,previous);m.word(tail,p);m.word(used+4,p);m.word(p+4,tail);m.word(p,used);
  const auto length=std::strlen(name)+1;::port::driving::Memory::Temporary text(m,static_cast<std::uint32_t>(length));for(unsigned i=0;i<length;++i)m.byte(text.address()+i,static_cast<unsigned char>(name[i]));hp2::debug::ReplaySnapshot(m,p).Init(text.address());
  m.word(p+0x150,0);m.byte(p+0x3e,1);HandleSnapshot(p,services);m.byte(p+0x3e,0);m.word(p+0x154,m.word(p+0x150));
 }
}
// Debug0013DB00; E-0099-W74; first matching configured loop wins.
void ReplayManager::MaybeDoChecksumSnapshot(const ::port::race::SnapshotServices& services){for(int i=0;i<4;++i)if(m_.word(0x327d10)==m_.word(0x2e8168+4*i)){char name[32];std::snprintf(name,sizeof(name),"CHECKSUM_%d",i);DoChecksumSnapshot(name,services);return;}}
// Debug0013E2A0; E-0099-W81. ReplaySpecialEventInfo time is+4, not event+0.
// Counterevidence: W74 native tests exercised only empty lists; fresh MIPS
// 0013E2D0/314 points at table+40B8, and0013E340 reads info+4.
void ReplayManager::CheckForSpecialEvents(){
 auto& m=m_;const auto a=address_;int count=m.signed_word(a+0x40ac);if(!count)return;
 if(count<0||count>256)hp2::not_implemented("0x0013E2A0","Replay special-event count outside source array");
 int i=m.signed_word(a+0x40b0);if(i>=256)hp2::not_implemented("0x0013E2A0","Replay special-event cursor outside original physical table");
 const auto loop=m.signed_word(0x327d10);while(i>=0&&m.signed_word(a+0x40b8+16*i)>=loop)--i;if(i<0)i=0;
 if(i<count&&m.signed_word(a+0x40b8+16*i)<=m.signed_word(0x327d10)){
  for(;;){const auto info=a+0x40b4+16*i;if(m.signed_word(info+4)==m.signed_word(0x327d10)){HandleSpecialEvent(info);count=m.signed_word(a+0x40ac);}
   ++i;if(i>=count||m.signed_word(0x327d10)<m.signed_word(a+0x40b8+16*i))break;
  }
 }
 if(count-1<i)i=count-1;
 m.word(a+0x40b0,static_cast<std::uint32_t>(i));
}
// Debug0013D680; E-0099-W74; exact source order, no fabricated forced capture.
void ReplayManager::BeginFrame(const ::port::race::SnapshotServices& services){
 auto& m=m_;const auto a=address_;const auto loop=m.signed_word(0x327d10);
 if(m.word(a+0x60)){
  if(m.word(0x2e81f0))m.word(a,1);
  else if(loop==m.signed_word(a+0x50)){
   if(m.word(a+0xc))m.word(a,1);
   else if(!m.word(a+0x54))m.word(a+4,1);
   else if(m.signed_word(a+0x4c)<=loop&&loop<m.signed_word(a+0x50))m.word(a,1);
  }else if(m.signed_word(a+0x4c)<=loop&&loop<m.signed_word(a+0x50))m.word(a,1);
 }
 if(m.word(a+4)){MaybeTakeInstantReplaySnapshot(services);if(m.word(a+4))MaybeTakeLapSnapshot(services);}
 if(m.word(a)){if(m.word(a+0xc))m.word(a+0x14,1);else if(m.word(a+0x10))m.word(a+0x14,m.signed_word(0x327cf4)%4!=0);CheckForSpecialEvents();}
 MaybeTakeDummySnapshot(services);MaybeDoChecksumSnapshot(services);hp2::debug::DemoModeReplay(m,0x2e81f0).ServiceRecording(m.word(0x327d10)==m.word(a+0xa0),services);
}
// Debug0013DE08 +inline SetTime0013EB50; E-0099-W74. Capture takes priority over replay reset.
void ReplayManager::EndFrame(){auto& m=m_;const auto a=address_;if(m.word(a+4)){m.word(a+0x50,m.word(0x327d10)+1);float delta=m.scalar(0x327d08)*4000.f;delta=delta+0.5f;const auto packed=::port::driving::cvt_w_s(delta);m.word(a+4,0);m.word(a+0x5c,m.word(0x327cf8)+static_cast<std::uint32_t>(packed));}else if(m.word(a)){m.word(a+0x14,0);m.word(a,0);}}
}
namespace hp2::debug {
// Debug0013F2F0; E-0099-W74; source normal recording component. External save,
// erase/re-sort operations keep explicit reached boundaries.
void DemoModeReplay::ServiceRecording(bool took_snapshot,const ::port::race::SnapshotServices& services){
 auto& m=m_;const auto a=address_;hp2::recovered::ReplayManager replay(m,m.word(0x3271a8));
 if(took_snapshot){
  if(m.word(a+0x10)&&!m.word(a+0x18)){replay.DeleteSnapshot("REPLAY_MANAGER_STATE");replay.TakeSnapshot("REPLAY_MANAGER_STATE",services);m.word(a+0x18,replay.TakeSnapshot("DEMO_MODE_SNAPSHOT",services));}
  if(m.word(a+0x14)){if(const auto playing=m.word(a+0x20)){
   if(m.signed_word(0x327cf8)<static_cast<std::int32_t>(m.word(playing+0x28)+12000))hp2::not_implemented("0x0013F2F0","Active demo erase requires original SortSnapshots after replacement");
  }}
 }
 if(const auto p=m.word(a+0x18)){m.word(p+0x34,m.word(0x327d10));m.word(p+0x2c,m.word(0x327cf8));}
 if(m.word(a+0x28))hp2::not_implemented("0x0013F2F0","Active original DemoModeReplay.Save/bRefreshTweaker file boundary");
 if(m.word(a+0x2c))hp2::not_implemented("0x0013F2F0","Active original EraseRecordedSnapshots/bRefreshTweaker boundary");
}
}

namespace hp2::recovered {
// Debug0013E538; E-0099-W74; replay-frame/0013e538.json. First matching named node.
int ReplayManager::RestoreSnapshot(const char* name,const ::port::race::SnapshotServices& services){
 const auto snapshot=FindSnapshot(name,0);if(snapshot)RestoreSnapshot(snapshot,services);return snapshot!=0;
}
// Debug0013E928; E-0099-W74; replay-frame/0013e928.json. Only original wrapper owns restore flag.
void ReplayManager::RestoreSnapshot(std::uint32_t snapshot,const ::port::race::SnapshotServices& services){
 m_.word(snapshot+0x150,0);m_.byte(snapshot+0x3d,1);HandleSnapshot(snapshot,services);m_.byte(snapshot+0x3d,0);
 m_.word(address_+8,0);m_.word(address_+0xa0,m_.word(0x327d10));
}
}

#include "game_core/PauseMenu.hpp"
namespace hp2::recovered {
// Debug0013E298; E-0099-W81. Actual J0016F4D8/MOVE a0,a1 thunk; no dummy acknowledgement.
void ReplayManager::HandleSpecialEvent(std::uint32_t info){(void)hp2::debug::HandlePauseMenuReplaySpecialEvents(m_,info);}
}

namespace hp2::recovered {
// Debug0013E1E8; E-0112-P03. Capture before synchronous original handler.
void ReplayManager::AddSpecialEvent(int event,int param1,int param2){
 auto& m=m_;const auto a=address_;::port::driving::Memory::Temporary storage(m,16);const auto info=storage.address();m.word(info,std::uint32_t(event));m.word(info+4,m.word(0x327d10));m.word(info+8,std::uint32_t(param1));m.word(info+12,std::uint32_t(param2));
 if(m.signed_word(a+0x4c)!=-1&&!m.word(a+0x54)&&m.word(a+0x60)){const auto index=m.signed_word(a+0x40ac);if(index<0x100){if(index<0)hp2::not_implemented("0x0013E1E8","negative source replay event count");const auto to=a+0x40b4+16*unsigned(index);for(unsigned i=0;i<4;++i)m.word(to+4*i,m.word(info+4*i));m.word(a+0x40ac,std::uint32_t(index+1));}else MarkTheEndOfTime();}
 HandleSpecialEvent(info);
}
}
