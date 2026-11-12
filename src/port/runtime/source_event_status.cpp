#include "source_event_status.hpp"
#include "event_language_asset.hpp"
#include "game_core/port/driving/original_image.hpp"
#include "game_core/indep/inc/EventTree.hpp"
#include <algorithm>
#include <stdexcept>
namespace port::runtime {
SourceEventStatus::SourceEventStatus(const std::filesystem::path& project,assets::Source& assets,const std::string& front_path,const hp2::memorycard::Profile_002FC050& profile,int language)
 :memory_(driving::read_debug_image(driving::debug_image_path(project))),resources_(memory_){
 const auto front=assets.read(front_path);retail_trees_=hp2::frontend::parse_event_trees(front);const auto trees=race::parse_debug_event_tables(front);std::vector<EventTreeDefinition> order;
 for(const auto& t:trees){EventTreeDefinition d;d.tree_type=t.nType;for(const auto& e:t.pEvents)d.events.push_back({e.nNumber,e.nType});order.push_back(std::move(d));}
 race::EventProfileTransfer transfer;transfer.layout=build_event_profile_layout(EventProfileVersion::Retail,order);
 for(int t=0;t<2;++t)std::copy_n(profile.EventBlock_001FE988(t),256,transfer.blocks[t].begin());
 resources_.load(assets.read("GLOBAL/GLOBALB.BUN"),front,assets.read(event_language_asset(language)),transfer);
 memory_.word(0x2f3890+0x170,unsigned(language));explicit_goal_=memory_.allocate(8);
}
SourceEventStatus::~SourceEventStatus(){resources_.close();memory_.release(explicit_goal_);}
int SourceEventStatus::status(int tree_type,int number,const hp2::frontend::EventLink_002008B8* goal)const{
 unsigned tree=0;for(const auto candidate:resources_.events().trees())if(memory_.signed_word(candidate+8)==tree_type)tree=candidate;
 const auto info=tree?hp2::debug::EventTree::GetEventNumber(memory_,tree,number):0;
 if(!info)throw std::invalid_argument("Source status event identity is unavailable");
 if(goal){memory_.half(explicit_goal_,goal->field_0x00);memory_.half(explicit_goal_+2,goal->prerequisite_number_0x02);memory_.word(explicit_goal_+4,goal->goal_threshold_0x04);}
 // Original001FC2A8 reads both actual authored trees/global default flag and
 // source state. Explicit goals bypass unlock dependencies as in source.
 return hp2::debug::EventInfo::GetStatus(memory_,info,goal?explicit_goal_:0);
}
hp2::memorycard::Profile_002FC050 SourceEventStatus::formatter_profile(const hp2::memorycard::Profile_002FC050& original)const{
 auto out=original;
 for(const auto tree:resources_.events().trees()){
  const auto type=memory_.signed_word(tree+8);const auto info=hp2::debug::EventTree::GetEventNumber(memory_,tree,32);
  const auto found=std::find_if(retail_trees_.begin(),retail_trees_.end(),[&](const auto& t){return t.type_0x08==type;});
  if(!info||found==retail_trees_.end())throw std::logic_error("Shared source formatter event is missing");
  const auto row=found->FindLeafByEventNumber(32);const auto layout=hp2::frontend::build_profile_layout_001FE988(*found);
  out.EventBlock_001FE988(type)[layout.offsets.at(unsigned(row))]=memory_.byte(memory_.word(info+4));
 }
 return out;
}
}
