#include "event_reward_codec.hpp"
#include "game_core/port/driving/memory.hpp"
#include <stdexcept>
namespace port::runtime {
namespace {
// E-0102-RP01 manifest: each entry is the Debug physical slot for Retail0..16.
// Unique reward-car identity correspondence; thresholds/rules are not copied.
constexpr std::array<unsigned,17> common_points{{0,1,2,3,4,6,8,9,10,11,12,13,14,15,16,17,18}};
std::uint32_t get(const std::uint8_t* p){return std::uint32_t(p[0])|std::uint32_t(p[1])<<8|std::uint32_t(p[2])<<16|std::uint32_t(p[3])<<24;}
void put(std::uint8_t* p,std::uint32_t v){for(unsigned i=0;i<4;++i)p[i]=std::uint8_t(v>>(8*i));}
void retail_extent(const std::vector<std::uint8_t>& b){if(b.size()!=0x114e0)throw std::invalid_argument("Reward Retail profile extent differs from reviewed version");}
std::uint32_t at(std::uint32_t fred,unsigned offset,unsigned size){const auto value=std::uint64_t(fred)+offset;if(!fred||value+size>0x100000000ull)throw std::invalid_argument("Reward logical address wraps");return std::uint32_t(value);}
}
EventRewardTransfer read_debug_event_rewards(const driving::Memory& m,std::uint32_t fred){EventRewardTransfer out;for(unsigned i=0;i<10;++i)out.task_words[i]=m.word(at(fred,0x106b4+4*i,4));for(unsigned i=0;i<19;++i)out.point_words[i]=m.word(at(fred,0x106dc+4*i,4));return out;}
void write_debug_event_rewards(driving::Memory& m,std::uint32_t fred,const EventRewardTransfer& value){auto* task=m.view(at(fred,0x106b4,40),40).data;auto* point=m.view(at(fred,0x106dc,76),76).data;for(unsigned i=0;i<10;++i)put(task+4*i,value.task_words[i]);for(unsigned i=0;i<19;++i)put(point+4*i,value.point_words[i]);}
EventRewardTransfer import_retail_event_rewards(const std::vector<std::uint8_t>& retail,const EventRewardTransfer& baseline){retail_extent(retail);auto out=baseline;for(unsigned i=0;i<10;++i)out.task_words[i]=get(retail.data()+0x106b8+4*i);for(unsigned i=0;i<common_points.size();++i)out.point_words[common_points[i]]=get(retail.data()+0x106e0+4*i);return out;}
std::vector<std::uint8_t> project_retail_event_rewards(const EventRewardTransfer& value,const std::vector<std::uint8_t>& baseline,bool final_award){retail_extent(baseline);auto out=baseline;for(unsigned i=0;i<10;++i)put(out.data()+0x106b8+4*i,value.task_words[i]);for(unsigned i=0;i<common_points.size();++i)put(out.data()+0x106e0+4*i,value.point_words[common_points[i]]);if(final_award)put(out.data()+0x106e0+4*17,1);return out;}
std::vector<std::uint8_t> encode_debug_reward_payload(const EventRewardTransfer& value){std::vector<std::uint8_t> out(116);for(unsigned i=0;i<10;++i)put(out.data()+4*i,value.task_words[i]);for(unsigned i=0;i<19;++i)put(out.data()+40+4*i,value.point_words[i]);return out;}
EventRewardTransfer decode_debug_reward_payload(const std::vector<std::uint8_t>& bytes){if(bytes.size()!=116)throw std::invalid_argument("Debug reward physical payload must be exactly116 bytes");EventRewardTransfer out;for(unsigned i=0;i<10;++i)out.task_words[i]=get(bytes.data()+4*i);for(unsigned i=0;i<19;++i)out.point_words[i]=get(bytes.data()+40+4*i);return out;}
}
