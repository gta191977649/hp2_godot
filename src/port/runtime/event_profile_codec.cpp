#include "event_profile_codec.hpp"
#include <algorithm>
#include <map>
#include <optional>
#include <stdexcept>
#include <utility>
namespace port::runtime {
namespace {
std::size_t state_size(EventProfileVersion version,int type){
 if(version!=EventProfileVersion::Retail&&version!=EventProfileVersion::Debug)throw std::invalid_argument("Unknown event profile version");
 switch(type){case 0:case 2:return 1;case 1:case 5:case 6:return 4;case 3:return version==EventProfileVersion::Retail?16:12;case 4:return 16;default:throw std::invalid_argument("Event profile type lacks reviewed version correspondence");}
}
void validate(const EventProfileLayout& layout){
 std::optional<EventStateLocation> shared;
 // Occupancy keys distinguish the sole source-proven alias from all unrelated
 // event states. Overlapping malformed layouts must never corrupt a baseline.
 std::array<std::array<int,256>,2> occupied{};
 for(int tree=0;tree<2;++tree){
  if(layout.trees[tree].size()!=32)throw std::invalid_argument("Event profile requires all32 authored records per tree");
  std::array<bool,33> numbers{};
  for(const auto& entry:layout.trees[tree]){
   const auto number=entry.event.number;const auto where=entry.location;
   if(number<1||number>32||numbers[number])throw std::invalid_argument("Duplicate or unsupported authored event number");numbers[number]=true;
   if(entry.size!=state_size(layout.version,entry.event.type)||where.tree_type<0||where.tree_type>1||where.offset>256||entry.size>256-where.offset)throw std::invalid_argument("Event profile state extent/version mismatch");
   if(number==32){if(entry.event.type!=0)throw std::invalid_argument("Shared event32 is outside the reviewed authored rank domain");if(shared&&!(*shared==where))throw std::invalid_argument("Original shared event32 alias was split");shared=where;}
   else if(where.tree_type!=tree)throw std::invalid_argument("Unreviewed cross-tree state alias");
   const int key=number==32?1000:1+tree*32+number;
   for(std::size_t i=0;i<entry.size;++i){auto& old=occupied[where.tree_type][where.offset+i];if(old&&old!=key)throw std::invalid_argument("Overlapping unrelated event profile states");old=key;}
  }
 }
}
const EventStateLayout& find(const EventProfileLayout& layout,int tree,int number){
 const auto& entries=layout.trees[tree];const auto it=std::find_if(entries.begin(),entries.end(),[&](const auto& item){return item.event.number==number;});
 if(it==entries.end())throw std::invalid_argument("Destination event profile identity absent");return *it;
}
}
EventProfileLayout build_event_profile_layout(EventProfileVersion version,const std::vector<EventTreeDefinition>& order){
 if(order.size()!=2)throw std::invalid_argument("Both original event trees and loader order are required");
 EventProfileLayout out{version,{},{{0,0}}};std::array<bool,2> seen{{false,false}};std::optional<EventStateLocation> shared;
 for(const auto& tree:order){
  const int type=tree.tree_type;if(type<0||type>1||seen[type]||tree.events.size()!=32)throw std::invalid_argument("Invalid authored tree identity/count/load order");seen[type]=true;
  auto& entries=out.trees[type];entries.resize(tree.events.size());
  for(int pass=0;pass<2;++pass)for(std::size_t i=0;i<tree.events.size();++i){
   const auto definition=tree.events[i];const auto size=state_size(version,definition.type);if((pass==0)!=(size>=4))continue;
   auto& entry=entries[i];entry.event=definition;entry.size=size;
   if(definition.number==32&&shared){entry.location=*shared;continue;}
   entry.location={type,out.used[type]};out.used[type]+=size;
   if(pass==0)out.used[type]=(out.used[type]+3)/4*4;
   if(out.used[type]>256)throw std::invalid_argument("Original event state allocation exceeds its256-byte block");
   if(definition.number==32)shared=entry.location;
  }
 }
 validate(out);return out;
}
EventProfilePayload transcode_event_profile(const EventProfileLayout& from,const EventProfilePayload& source,
 const EventProfileLayout& to,const EventProfilePayload& baseline){
 validate(from);validate(to);auto result=baseline;
 for(int tree=0;tree<2;++tree)for(const auto& entry:from.trees[tree]){
  const auto& target=find(to,tree,entry.event.number);if(target.event.type!=entry.event.type)throw std::invalid_argument("Event profile type correspondence changed");
  const auto* input=source[entry.location.tree_type].data()+entry.location.offset;
  auto* output=result[target.location.tree_type].data()+target.location.offset;
  const auto copy=[&](std::size_t src,std::size_t dst,std::size_t count){std::copy_n(input+src,count,output+dst);};
  switch(entry.event.type){
  case 0:case 2:copy(0,0,1);break;
  case 1:case 5:case 6:copy(0,0,4);break;
  case 3:{
   // Same packed eight rank nibbles; different selected-car/seed offsets.
   // Retail+4..7 has no reviewed Debug counterpart and remains in baseline.
   const std::size_t src=from.version==EventProfileVersion::Retail?8:4,dst=to.version==EventProfileVersion::Retail?8:4;
   copy(0,0,4);copy(src,dst,2);copy(src+2,dst+2,1);copy(src+3,dst+3,1);copy(src+4,dst+4,4);break;
  }
  case 4:copy(0,0,8);copy(8,8,2);copy(10,10,1);copy(11,11,1);copy(12,12,4);break;
  default:throw std::logic_error("Unvalidated event profile type reached codec");
  }
 }
 return result;
}
}
