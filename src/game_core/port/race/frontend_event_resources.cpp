#include "frontend_event_resources.hpp"
#include "game_core/indep/inc/EventTree.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include "game_core/indep/src/frontendscreens/campaigntrees.hpp"
#include "game_core/port/assets/elab_chunks.hpp"
#include <cstring>
#include <stdexcept>
#include <algorithm>
namespace port::race {
void build_frontend_unlock_map(driving::Memory& m,std::uint32_t tree,std::uint32_t info,std::uint32_t map){
 if(m.word(info)!=tree||hp2::debug::EventTree::GetEventNumber(m,tree,m.signed_byte(info+0x28))!=info)throw std::logic_error("Unlock-map request must reference the first actual authored event row");
 hp2::debug::tUnlockMap::Reset(m,map);const int parent=m.signed_byte(info+0x28);
 // Source001F5060 appends child edges in EventInfo/goal order, including
 // duplicates. Source001F3630 then revisits every matching goal per edge.
 for(unsigned row=0;row<m.word(tree+0x44);++row){const auto child=m.word(tree+0x3c)+row*0xbc;
  for(int edge=0;edge<m.signed_byte(child+0x3d);++edge){const auto trigger=hp2::debug::EventInfo::GetGoal(m,child,edge);if(int(m.half(trigger+2))!=parent)continue;
   (void)hp2::debug::EventInfo::GetObjective(m,info);
   for(int index=0;index<m.signed_byte(child+0x3d);++index){const auto goal=hp2::debug::EventInfo::GetGoal(m,child,index);if(int(m.half(goal+2))==parent)hp2::debug::tUnlockMap::AddGoal(m,map,goal,m.signed_byte(child+0x28));}
  }
 }
}
void initialize_frontend_event_resource_state(driving::Memory& m){
 if(m.word(0x3287f0)||m.word(0x3287f4)||m.word(0x2f3890+0x384))throw std::logic_error("Fresh original frontend event startup required");
 for(unsigned i=0;i<512;++i)if(m.byte(0x2f3890+0x184+i))throw std::logic_error("Existing raw Fred progress requires its original profile owner; no decoded profile substitution");
 // Source InitWithDefaults181D78 event-state component, not a whole profile.
 for(unsigned i=0;i<512;++i){m.byte(0x2f3890+0x184+i,0);}m.word(0x2f3890+0x384,0);
 // Source CRT1FD868 list and icon-hash component, not its other UI globals.
 m.word(0x3287f0,0x3287f0);m.word(0x3287f4,0x3287f0);
 const char* names[]={"RACEICON","TIMETRIALICON","LAPKNOCKOUTICON","RACEKNOCKOUTICON","TOURNEYICON","POINT2POINTICON"};
 for(unsigned i=0;i<6;++i)m.word(0x312fc0+i*4,hp2::frontend::bStringHash(names[i]));
}
void FrontendEventResources::load_bundle(const std::vector<std::uint8_t>& bytes,std::optional<std::uint32_t> center_w){
 auto& m=memory_;if(!chunks_.empty()||m.word(0x3287f0)!=0x3287f0||m.word(0x3287f4)!=0x3287f0)throw std::logic_error("Original EventTree owner must be initialized and empty");
 std::size_t consumed=0;const auto chunks=hp2::assets::parse_chunks_top(bytes,&consumed);if(consumed!=bytes.size())throw std::runtime_error("Incomplete FRONTB source chunk stream");
 const auto visit=[&](const auto& self,const std::vector<hp2::assets::Chunk>& items)->void{for(const auto& chunk:items){if(chunk.tag==0x80034a00){
   // Exact common authored188-byte EventInfo schema reviewed in E-0097-D01.
   // Keep the complete bytes including opaque source fields, then invoke the
   // original loader. No serialized pointer is interpreted as a host pointer.
   for(const auto& child:chunk.children){const unsigned stride=child.tag==0x34a05?0xbc:((child.tag==0x34a03||child.tag==0x34a04)?8:((child.tag>=0x34a01&&child.tag<=0x34a06)?4:1));if(child.size%stride)throw std::runtime_error("Misaligned authored EventTree source table");}
   const auto prefix=std::uint32_t(chunk.header_offset&15),length=chunk.size+8,allocation=m.allocate(length+prefix),at=allocation+prefix;allocations_.push_back(allocation);chunks_.push_back(at);std::memcpy(m.view(at,length).data,bytes.data()+chunk.header_offset,length);
   hp2::debug::LoaderEventTree(m,at,center_w);
  }else self(self,chunk.children);}};visit(visit,chunks);
 for(auto tree=m.word(0x3287f0);tree!=0x3287f0;tree=m.word(tree)){if(m.word(tree+0x48)>256)throw std::runtime_error("Authored event progress exceeds original Fred256-byte state buffer");trees_.push_back(tree);}
}
void FrontendEventResources::close(){auto& m=memory_;
 if(std::find(trees_.begin(),trees_.end(),m.word(0x312cd0))!=trees_.end())throw std::logic_error("Original selected RaceEvent still borrows its EventTree owner");
 if(!chunks_.empty()){hp2::debug::UnloaderEventTree(m,chunks_[0]);}for(auto allocation:allocations_)m.release(allocation);allocations_.clear();chunks_.clear();trees_.clear();
}
void FrontendEventResources::release_race_event(){auto& m=memory_;if(m.word(0x327290)||m.word(0x3272f0)||m.word(0x327258))throw std::logic_error("Selected raw event still belongs to live native race owners");if(std::find(trees_.begin(),trees_.end(),m.word(0x312cd0))!=trees_.end()){m.word(0x312cd0,0);m.word(0x312cd4,0);}}
}
