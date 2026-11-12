#include "event_world_launch.hpp"
#include "event_runtime_continuation.hpp"
#include "game_core/indep/inc/Fred.hpp"
#include "game_core/indep/inc/EventTree.hpp"
#include "game_core/indep/src/eventtree.hpp"
#include "game_core/port/not_implemented.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace port::race {
namespace {
constexpr std::uint32_t fred=0x2f3890,rp=0x311f78;
using ::port::runtime::EventProfileLayout;
using ::port::runtime::EventProfilePayload;
using ::port::runtime::EventProfileVersion;
std::vector<::port::runtime::EventTreeDefinition> profile_definitions(const driving::Memory& m,const FrontendEventResources& resources){
 std::vector<::port::runtime::EventTreeDefinition> order;
 for(const auto tree:resources.trees()){::port::runtime::EventTreeDefinition definition;definition.tree_type=m.signed_word(tree+8);for(unsigned row=0;row<m.word(tree+0x44);++row){const auto info=m.word(tree+0x3c)+row*0xbc;definition.events.push_back({m.signed_byte(info+0x28),m.signed_byte(info+0x29)});}order.push_back(std::move(definition));}
 return order;
}
bool same_layout(const EventProfileLayout& a,const EventProfileLayout& b){
 if(a.version!=b.version||a.used!=b.used)return false;
 for(int tree=0;tree<2;++tree){if(a.trees[tree].size()!=b.trees[tree].size())return false;for(std::size_t row=0;row<a.trees[tree].size();++row){const auto& x=a.trees[tree][row];const auto& y=b.trees[tree][row];if(x.event.number!=y.event.number||x.event.type!=y.event.type||x.size!=y.size||!(x.location==y.location))return false;}}
 return true;
}
EventProfilePayload debug_blocks(const driving::Memory& m){EventProfilePayload blocks;for(unsigned tree=0;tree<2;++tree)for(unsigned i=0;i<256;++i)blocks[tree][i]=m.byte(fred+0x184+tree*256+i);return blocks;}
EventProfileLayout checked_debug_layout(const driving::Memory& m,const FrontendEventResources& resources,const EventProfileTransfer& original){
 const auto definitions=profile_definitions(m,resources);
 const auto expected=::port::runtime::build_event_profile_layout(EventProfileVersion::Retail,definitions);
 if(!same_layout(expected,original.layout))throw std::invalid_argument("Authored Retail profile layout differs from original loaded tree order/records");
 const auto debug=::port::runtime::build_event_profile_layout(EventProfileVersion::Debug,definitions);
 for(const auto tree:resources.trees()){const auto type=m.signed_word(tree+8);for(unsigned row=0;row<m.word(tree+0x44);++row){const auto info=m.word(tree+0x3c)+row*0xbc;const auto where=debug.trees.at(type).at(row).location;if(m.word(info+4)!=fred+0x184+unsigned(where.tree_type)*256+where.offset)throw std::logic_error("Semantic Debug layout differs from original CalcStatePointers including shared32");}}
 return debug;
}
std::uint32_t selected_tree(const driving::Memory& m,const EventLaunchRequest& event,const FrontendEventResources& resources){
 for(auto tree:resources.trees())if(m.signed_word(tree+8)==event.tree_type)return tree;
 throw std::invalid_argument("Requested authored event tree is unavailable");
}
// The selected profile block is a semantic sequence of source event states.
// Retail and Debug independently calculate their offsets from the same reviewed
// rank/time asset records. Never copy a whole version-specific Fred/profile.
void transfer_progress(driving::Memory& m,const EventLaunchRequest& request,std::uint32_t tree,const FrontendEventResources& resources){
 if(request.event_progress.size()!=256)throw std::invalid_argument("Authored launch requires the complete selected-tree progress block");
 if(request.event_profile){
  if(!std::equal(request.event_progress.begin(),request.event_progress.end(),request.event_profile->blocks.at(request.tree_type).begin()))throw std::invalid_argument("Selected legacy profile differs from both-tree authored transport");
  const auto layout=checked_debug_layout(m,resources,*request.event_profile);
  const auto converted=::port::runtime::transcode_event_profile(request.event_profile->layout,request.event_profile->blocks,layout,debug_blocks(m));
  for(unsigned t=0;t<2;++t)for(unsigned i=0;i<256;++i)m.byte(fred+0x184+t*256+i,converted[t][i]);
  return;
 }
 std::vector<std::uint8_t> source=request.event_progress;
 for(unsigned row=0;row<m.word(tree+0x44);++row){const auto info=m.word(tree+0x3c)+row*0xbc;const int type=m.signed_byte(info+0x29),size=hp2::debug::EventInfo::GetStateSize(m,info);
  // KO/tournament layouts require an independently reviewed version adapter.
  // A fresh all-zero state has the same reset meaning; nonzero is not overlaid.
  const auto base=fred+0x184,at=m.word(info+4);
  // Source event32 may alias the other tree's state through Fred+384.
  if(at<base||at+unsigned(size)>base+512)throw std::logic_error("Source event progress points outside Fred event buffers");
  const auto offset=request.event_progress_offsets.at(row);
  if(offset<0&&m.signed_byte(info+0x28)==32)continue; // Shared other-tree state has no selected-block slice.
  if(offset<0||offset+size>256)throw std::invalid_argument("Retail event-state offset exceeds selected profile block");
  if(type!=0&&type!=1&&type!=2&&type!=5&&type!=6){if(std::any_of(source.begin()+offset,source.begin()+offset+size,[](auto v){return v!=0;}))hp2::not_implemented("0x001F9F88","nonzero cross-version KO/tournament profile transport is not reviewed");}
  std::memcpy(m.view(at,size).data,source.data()+offset,size);
 }
}
}
void prepare_event_world_launch(driving::Memory& m,WorldLaunchRequest& request,const LaunchCatalog& catalog,FrontendEventResources& resources){
 if(!request.event)throw std::invalid_argument("Production event request is absent");auto event=*request.event;
 // A source RC9 continuation already executed SetupNextRace/RestartEvent.
 // Its prepared binding is only a destination-resource rebind; mutable source
 // state is restored below. Never rerun the initial menu SetRace/StartRace.
 if(request.continuation)event.prepared_event=request.continuation->prepared_event;
 if(event.tree_type<0||event.tree_type>1||event.event_index<0||event.car_id<0||event.car_id>=64)throw std::invalid_argument("Invalid authored event selection identity");
 const auto tree=selected_tree(m,event,resources);if(unsigned(event.event_index)>=m.word(tree+0x44))throw std::invalid_argument("Authored event row exceeds its tree");
 const auto info=m.word(tree+0x3c)+unsigned(event.event_index)*0xbc;
 if(m.signed_byte(info+0x28)!=event.event_number)throw std::invalid_argument("Authored event number does not match its row");
 hp2::debug::FrontendDatabase::Sub_00181BE8(m,fred);hp2::debug::HighScoreTable::InitStatsTables(m,fred+0x6b4);
 if(event.event_scores){
  const auto axes=::port::runtime::read_debug_event_score_axes(m);
  const auto scores=::port::runtime::transcode_event_scores(event.event_scores->axes,event.event_scores->profile,axes);
  ::port::runtime::write_debug_event_scores(m,fred,axes,scores);
 }else if(event.prepared_event)throw std::invalid_argument("Production prepared event omitted its score/name baseline");
 if(event.event_rewards)::port::runtime::write_debug_event_rewards(m,fred,*event.event_rewards);
 if(event.prepared_event)::port::runtime::initialize_prepared_event_parameters(m);
 else hp2::debug::RaceParameters(m,rp).InitWithDefaults({});
 transfer_progress(m,event,tree,resources);m.word(0x329060,event.random_state);
 if(hp2::debug::EventInfo::GetStatus(m,info)==hp2::debug::eEVENT_LOCKED)throw std::invalid_argument("Selected authored event is locked in the actual source progress");
 const int mode=event.tree_type==0?8:9;m.word(fred+0x10734,1);m.word(fred+0x10,1);m.word(fred+0x170,unsigned(event.language));m.word(rp+0x254,1); // Source StartEvent001F2BD8 modes; SelectedLanguage is independent.
 m.word(fred+0x1072c,unsigned(mode));m.word(fred+0x160,unsigned(event.tree_type));m.word(fred+0x160+(event.tree_type==0?8:4),unsigned(event.event_number));m.word(fred+0x10e04,event.player_points);
 // Original StartEvent001F2BD8 data/goal producers, as E-0099-W88.
 if(event.prepared_event){
  if(!event.event_profile)throw std::invalid_argument("Prepared menu launch requires complete post-SetRace profile transport");
  if(event.prepared_event->tree_type!=event.tree_type||event.prepared_event->event_number!=event.event_number||event.prepared_event->event_type!=m.signed_byte(info+0x29))throw std::invalid_argument("Prepared menu event identity differs from authored selection");
  ::port::runtime::bind_prepared_event(m,resources,*event.prepared_event);
 }else hp2::debug::EventTree::SetRace(m,tree,rp,info); // Explicit legacy/native fixtures only.
 build_frontend_unlock_map(m,tree,info,0x312cd0+0x1b0);
 if(request.continuation){
  restore_event_runtime_continuation(m,*request.event,*request.continuation);
  const auto& c=*request.continuation;
  if(!catalog.skin_info(c.car_id,c.skin_id,0))throw std::invalid_argument("Continued original event skin is unavailable");
  request.track=c.track;request.reverse=c.reverse;request.car_id=c.car_id;request.skin_id=c.skin_id;
  const auto& name=catalog.cars()[c.car_id].CarTypeName;request.car_code.assign(name.data(),std::find(name.begin(),name.end(),'\0')-name.begin());
  return;
 }
 hp2::debug::FrontendDatabase::MakeEventCarsAvailable(m,fred,tree,info);
 if(m.word(hp2::debug::FrontendPlayerInfo::GetCarStatus(m,fred+0x10738,event.car_id))!=hp2::debug::eCAR_UNLOCKED_FULLY_VISIBLE)throw std::invalid_argument("Selected car is unavailable in this original event");
 if(!catalog.skin_info(event.car_id,event.skin_id,0))throw std::invalid_argument("Selected original event skin is unavailable");
 if(event.event_settings){
  const auto& p=event.event_settings->players[0];
  if(p.driving_style!=std::uint8_t(event.driving_style)||p.transmission_type!=std::uint8_t(event.transmission)||p.preferred_gender!=std::uint8_t(event.preferred_gender))throw std::invalid_argument("Full source settings conflict with legacy selected fields");
  ::port::runtime::write_debug_event_settings(m,fred,*event.event_settings);
 }else if(event.prepared_event)throw std::invalid_argument("Production prepared event omitted full player/display settings");
 for(unsigned player=0;player<2;++player){const auto data=fred+0x10738+player*0x6d0;
  if(!event.event_settings){hp2::debug::cPlayerSettings::ResetToDefaults(m,data+0x644,{});
  // E-0098-V02: individually reviewed PlayerSettings semantic fields.
  m.byte(data+0x646,unsigned(event.driving_style));m.byte(data+0x647,unsigned(event.transmission));m.byte(data+0x64b,unsigned(event.preferred_gender));}
  hp2::debug::FrontendPlayerInfo::AssignCarOfType(m,data,mode,event.car_id,unsigned(event.skin_id),event.has_dash?1:0);
  const auto ride=hp2::debug::FrontendPlayerInfo::GetSelectedRideInfo(m,data,mode);if(m.word(ride)!=unsigned(event.car_id)||m.byte(ride+5)!=event.skin_id)throw std::invalid_argument("Selected ride differs from the original event's available skins");
 }
 hp2::debug::FrontendDatabase::StartRace(m,fred);
 request.track=m.signed_word(rp);request.reverse=m.word(rp+0xc)!=0;request.car_id=event.car_id;request.skin_id=event.skin_id;
 const auto& name=catalog.cars()[event.car_id].CarTypeName;request.car_code.assign(name.data(),std::find(name.begin(),name.end(),'\0')-name.begin());
}
std::vector<std::uint8_t> export_event_progress(const driving::Memory& m,const EventLaunchRequest& event,const FrontendEventResources& resources){
 if(const auto full=export_event_profile(m,event,resources)){const auto& block=full->at(event.tree_type);return {block.begin(),block.end()};}
 const auto tree=selected_tree(m,event,resources);auto output=event.event_progress;const auto base=fred+0x184;
 for(unsigned row=0;row<m.word(tree+0x44);++row){const auto info=m.word(tree+0x3c)+row*0xbc;const auto at=m.word(info+4);const int size=hp2::debug::EventInfo::GetStateSize(m,info);if(at<base||at+unsigned(size)>base+512)throw std::logic_error("Source event export exceeds Fred event buffers");const auto offset=event.event_progress_offsets.at(row);if(offset<0&&m.signed_byte(info+0x28)==32)continue;if(offset<0||offset+size>256)throw std::invalid_argument("Retail event export exceeds selected profile block");for(int i=0;i<size;++i)output[unsigned(offset+i)]=m.byte(at+unsigned(i));}
 return output;
}
std::optional<::port::runtime::EventProfilePayload> export_event_profile(const driving::Memory& m,const EventLaunchRequest& event,const FrontendEventResources& resources){
 if(!event.event_profile)return {};
 const auto debug=checked_debug_layout(m,resources,*event.event_profile);
 return ::port::runtime::transcode_event_profile(debug,debug_blocks(m),event.event_profile->layout,event.event_profile->blocks);
}
std::optional<::port::runtime::EventScoreProfile> export_event_scores(const driving::Memory& m,const EventLaunchRequest& event){
 if(!event.event_scores)return {};
 const auto axes=::port::runtime::read_debug_event_score_axes(m);
 return ::port::runtime::transcode_event_scores(axes,::port::runtime::read_debug_event_scores(m,fred,axes),event.event_scores->axes);
}
std::optional<::port::runtime::EventSettingsTransfer> export_event_settings(const driving::Memory& m,const EventLaunchRequest& event){
 if(!event.event_settings)return {};
 return ::port::runtime::read_debug_event_settings(m,fred);
}
}
