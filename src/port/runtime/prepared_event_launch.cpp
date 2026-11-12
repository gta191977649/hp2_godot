#include "prepared_event_launch.hpp"
#include "game_core/indep/inc/EventTree.hpp"
#include "game_core/indep/inc/Fred.hpp"
#include "game_core/indep/src/raceparameters.hpp"
#include <stdexcept>
namespace port::runtime {
namespace {
constexpr unsigned event=0x312cd0,rp=0x311f78,fred=0x2f3890;
// Scalar addresses independently anchored to the exact Debug source writes;
// float/Timer payloads retain their original bits, no host conversion.
constexpr unsigned offsets[]={0x20,0x24,0x28,0x30,0x34,0x38,0x40,0x4c,0x50,
 0x254,0x25c,0x260,0x264,0x268,0x26c,0x270,0x274,0x278,0x27c,0x280,0x284,
 0x288,0x2fc,0x300,0x30c};
static_assert(sizeof(offsets)/sizeof(*offsets)==unsigned(PreparedParameter::Count));
std::string text(const driving::Memory& m,unsigned at,unsigned maximum){std::string out;for(unsigned i=0;i<maximum;++i){const auto c=m.byte(at+i);if(!c)return out;out+=char(c);}throw std::invalid_argument("Prepared source text exceeds original bounded buffer");}
void put_text(driving::Memory& m,unsigned at,const std::string& value,unsigned maximum){if(value.size()>=maximum)throw std::invalid_argument("Prepared text exceeds source storage");for(unsigned i=0;i<value.size();++i)m.byte(at+i,std::uint8_t(value[i]));m.byte(at+unsigned(value.size()),0);}
}
PreparedEventLaunch capture_prepared_event(const driving::Memory& m,std::uint32_t tree,std::uint32_t info){
 if(m.word(info)!=tree||m.word(event)!=tree||m.word(event+4)!=m.word(info+4)||m.word(tree+0x5c)!=info)throw std::logic_error("Prepared capture requires actual current source event ownership");
 PreparedEventLaunch out;out.tree_type=m.signed_word(tree+8);out.event_number=m.signed_byte(info+0x28);out.event_type=m.signed_byte(info+0x29);
 for(unsigned i=0;i<out.parameters.size();++i)out.parameters[i]=m.word(rp+offsets[i]);
 out.player_start_position=m.byte(rp+0x3c);out.objective=text(m,rp+0x54,512);out.reward_name=text(m,event+0x48,32);
 const auto size=hp2::debug::EventInfo::GetStateSize(m,info);for(int i=0;i<size;++i)out.old_state.push_back(m.byte(event+8+unsigned(i)));
 return out;
}
void initialize_prepared_event_parameters(driving::Memory& m){
 // E-0098-RP01 / E-0101-PL01. Port reconstruction of destination storage for
 // the SAME already initialized source RP, not another InitWithDefaults call.
 // Every baseline store below is source001E9968. RandomSeedForCarColors is
 // supplied by bind_prepared_event; no RNG state is read, reset or advanced.
 for(unsigned i=0;i<0x310;++i)m.byte(rp+i,0);
 m.word(rp,11);m.word(rp+8,3);m.scalar(rp+0x10,1);m.scalar(rp+0x14,.5f);
 m.word(rp+0x2c,2);m.word(rp+0x30,1);m.word(rp+0x34,7);
 m.byte(rp+0x3c,7);m.byte(rp+0x3d,8);m.word(rp+0x254,1);
 for(unsigned offset=0x260;offset<=0x270;offset+=4)m.word(rp+offset,1);
 m.word(rp+0x278,1);m.word(rp+0x27c,2);m.word(rp+0x2f8,1);
 m.scalar(rp+0x2fc,1);m.scalar(rp+0x300,1);
 for(unsigned i=0;i<0x150;++i)m.byte(0x312288+i,0);
 m.word(rp+0x304,0x312288);m.word(rp+0x308,0);
 hp2::debug::cPlayerSettings::ResetToDefaults(m,rp+0x28c,{});
 hp2::debug::cPlayerSettings::ResetToDefaults(m,rp+0x2bc,{});
 hp2::debug::tGlobalDisplaySettings::ResetToDefaults(m,rp+0x2ec,{});
}
void bind_prepared_event(driving::Memory& m,const race::FrontendEventResources& resources,const PreparedEventLaunch& prepared){
 unsigned tree=0,info=0;for(const auto candidate:resources.trees())if(m.signed_word(candidate+8)==prepared.tree_type)tree=candidate;
 if(tree)info=hp2::debug::EventTree::GetEventNumber(m,tree,prepared.event_number);
 if(!info||m.signed_byte(info+0x29)!=prepared.event_type||prepared.old_state.size()!=unsigned(hp2::debug::EventInfo::GetStateSize(m,info)))throw std::invalid_argument("Prepared event identity/state differs from destination authored resources");
 if(prepared.objective.size()>=512||prepared.reward_name.size()>=32)throw std::invalid_argument("Prepared event text exceeds source storage");
 // All guest pointers come from the destination owner. Source SetRace's immutable
 // event data projection is reconstructed from its actual EventInfo tables.
 m.word(tree+0x5c,info);m.word(fred+0x10730,unsigned(prepared.event_number));m.word(event,tree);m.word(event+4,m.word(info+4));
 for(unsigned i=0;i<prepared.old_state.size();++i)m.byte(event+8+i,prepared.old_state[i]);
 for(unsigned i=0;i<32;++i)m.byte(event+0x28+i,m.byte(info+8+i));
 const unsigned fields[][2]={{0x68,0x28},{0x6a,0x29},{0x6c,0x2a},{0x6e,0x3b},{0x70,0x3c},{0x72,0x3e},{0x74,0x3f}};
 for(const auto& f:fields){m.half(event+f[0],std::uint16_t(std::int16_t(m.signed_byte(info+f[1]))));}
 m.half(event+0x76,1);
 const auto objective=hp2::debug::EventInfo::GetObjective(m,info);m.word(event+0x7c,m.word(objective));m.word(event+0x80,m.word(objective+4));
 for(int i=0;i<m.signed_byte(info+0x3b);++i)m.word(event+0x84+4*unsigned(i),m.word(hp2::debug::EventInfo::GetCar(m,info,i)));
 for(int i=0;i<m.signed_byte(info+0x3c);++i)m.word(event+0xd4+4*unsigned(i),m.word(hp2::debug::EventInfo::GetTrack(m,info,i)));
 for(int i=0;i<m.signed_byte(info+0x3e);++i){const auto from=hp2::debug::EventInfo::GetReward(m,info,i);m.word(event+0xf4+8*unsigned(i),m.word(from));m.word(event+0xf8+8*unsigned(i),m.word(from+4));}
 for(int i=0;i<m.signed_byte(info+0x3f);++i)m.word(event+0x134+4*unsigned(i),m.word(hp2::debug::EventInfo::GetOpponent(m,info,i)));
 hp2::debug::RaceEvent::ResetResults(m,event);
 for(unsigned i=0;i<prepared.parameters.size();++i){m.word(rp+offsets[i],prepared.parameters[i]);}
 m.byte(rp+0x3c,prepared.player_start_position);
 put_text(m,rp+0x54,prepared.objective,512);put_text(m,event+0x48,prepared.reward_name,32);
}
}
