#include "selection_event_resources.hpp"
#include "game_core/port/race/damage_resources.hpp"
#include "game_core/indep/inc/CarInfo.hpp"
#include <stdexcept>
namespace port::runtime {
namespace {
constexpr unsigned fred=0x2f3890;
EventProfilePayload blocks(const driving::Memory& m){EventProfilePayload out;for(unsigned t=0;t<2;++t)for(unsigned i=0;i<256;++i)out[t][i]=m.byte(fred+0x184+t*256+i);return out;}
bool same(const EventProfileLayout& a,const EventProfileLayout& b){
 if(a.version!=b.version||a.used!=b.used)return false;
 for(unsigned t=0;t<2;++t){if(a.trees[t].size()!=b.trees[t].size())return false;for(unsigned i=0;i<a.trees[t].size();++i){const auto& x=a.trees[t][i];const auto& y=b.trees[t][i];if(x.event.number!=y.event.number||x.event.type!=y.event.type||x.size!=y.size||!(x.location==y.location))return false;}}return true;
}
}
SelectionEventResources::SelectionEventResources(driving::Memory& m):memory_(m),cars_(m),language_(m),events_(m),textures_(m){}
void SelectionEventResources::load(const std::vector<std::uint8_t>& global,const std::vector<std::uint8_t>& front,const std::vector<std::uint8_t>& language,const race::EventProfileTransfer& profile){
 if(loaded_)throw std::logic_error("Selection raw event resources already owned");
 // Validate metadata before allocating resources. Actual asset definitions and
 // original raw pointer ownership are independently checked after the loader.
 (void)transcode_event_profile(profile.layout,profile.blocks,profile.layout,profile.blocks);
 const auto before=memory_.word(0x329060);
 race::initialize_damage_resource_lists(memory_);race::initialize_render_resource_state(memory_);
 textures_.init();textures_.load_bundle(global);textures_.load_bundle(front);
 language_.load(language);language_.load_track_names(global);
 cars_.load_bundle(global);hp2::debug::InitCarInfo(memory_);
 race::initialize_frontend_event_resource_state(memory_);events_.load_bundle(front,0);
 std::vector<EventTreeDefinition> definitions;
 for(const auto tree:events_.trees()){EventTreeDefinition d;d.tree_type=memory_.signed_word(tree+8);for(unsigned i=0;i<memory_.word(tree+0x44);++i){const auto info=memory_.word(tree+0x3c)+i*0xbc;d.events.push_back({memory_.signed_byte(info+0x28),memory_.signed_byte(info+0x29)});}definitions.push_back(std::move(d));}
 retail_=build_event_profile_layout(EventProfileVersion::Retail,definitions);debug_=build_event_profile_layout(EventProfileVersion::Debug,definitions);
 if(!same(retail_,profile.layout))throw std::invalid_argument("Selection profile metadata differs from original asset/load order");
 for(const auto tree:events_.trees()){const auto type=memory_.signed_word(tree+8);for(unsigned i=0;i<memory_.word(tree+0x44);++i){const auto info=memory_.word(tree+0x3c)+i*0xbc;const auto at=debug_.trees.at(type).at(i).location;if(memory_.word(info+4)!=fred+0x184+unsigned(at.tree_type)*256+at.offset)throw std::logic_error("Selection Debug source progress pointer differs from semantic layout");}}
 const auto progress=transcode_event_profile(retail_,profile.blocks,debug_,blocks(memory_));
 for(unsigned t=0;t<2;++t)for(unsigned i=0;i<256;++i)memory_.byte(fred+0x184+t*256+i,progress[t][i]);
 retail_baseline_=profile.blocks;
 if(memory_.word(0x329060)!=before)throw std::logic_error("Selection raw resource bootstrap unexpectedly consumed original RNG");
 loaded_=true;
}
EventProfilePayload SelectionEventResources::export_profile()const{
 if(!loaded_)throw std::logic_error("Selection raw event resources are not loaded");
 return transcode_event_profile(debug_,blocks(memory_),retail_,retail_baseline_);
}
void SelectionEventResources::import_profile(const EventProfilePayload& profile){
 if(!loaded_)throw std::logic_error("Selection raw event resources are not loaded");
 const auto progress=transcode_event_profile(retail_,profile,debug_,blocks(memory_));
 for(unsigned t=0;t<2;++t)for(unsigned i=0;i<256;++i)memory_.byte(fred+0x184+t*256+i,progress[t][i]);
}
void SelectionEventResources::close(){
 if(!loaded_)return;
 events_.release_race_event();events_.close();cars_.close();language_.close();textures_.unload_all();loaded_=false;
}
}
