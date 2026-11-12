#pragma once
// TASK-0101. Port-only semantic profile wire format, never an original ABI.
#include "port/runtime/event_launch.hpp"
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>
#include <cstring>
#include <stdexcept>
namespace port::godot_bridge {
inline godot::Dictionary prepared_transfer(const ::port::runtime::PreparedEventLaunch& source){
 godot::Dictionary out;out["schema_version"]=1;out["tree_type"]=source.tree_type;out["event_number"]=source.event_number;out["event_type"]=source.event_type;
 godot::PackedInt64Array parameters;for(auto value:source.parameters)parameters.push_back(value);out["parameters"]=parameters;
 out["player_start_position"]=int(source.player_start_position);
 // Original localized byte strings must not be transcoded through Unicode.
 godot::PackedByteArray objective,reward;objective.resize(source.objective.size());reward.resize(source.reward_name.size());if(objective.size())std::memcpy(objective.ptrw(),source.objective.data(),source.objective.size());if(reward.size())std::memcpy(reward.ptrw(),source.reward_name.data(),source.reward_name.size());out["objective_bytes"]=objective;out["reward_name_bytes"]=reward;
 godot::PackedByteArray old;old.resize(source.old_state.size());if(old.size())std::memcpy(old.ptrw(),source.old_state.data(),source.old_state.size());out["old_state"]=old;return out;
}
inline ::port::runtime::PreparedEventLaunch prepared_transfer(const godot::Dictionary& input){
 if(int(input.get("schema_version",-1))!=1)throw std::invalid_argument("Unknown prepared event semantic schema");
 ::port::runtime::PreparedEventLaunch out;out.tree_type=int(input.get("tree_type",-1));out.event_number=int(input.get("event_number",-1));out.event_type=int(input.get("event_type",-1));
 if(out.tree_type<0||out.tree_type>1||out.event_number<1||out.event_number>32||out.event_type<0||out.event_type>6)throw std::invalid_argument("Prepared event identity outside reviewed authored domain");
 const godot::PackedInt64Array parameters=input.get("parameters",godot::PackedInt64Array());if(parameters.size()!=int(out.parameters.size()))throw std::invalid_argument("Prepared event requires all25 named source parameters");
 for(int i=0;i<parameters.size();++i){if(parameters[i]<0||std::uint64_t(parameters[i])>0xffffffffull)throw std::invalid_argument("Prepared parameter loses original uint32 bits");out.parameters[i]=std::uint32_t(parameters[i]);}
 const int start=input.get("player_start_position",-1);if(start<0||start>255)throw std::invalid_argument("Prepared start position is not a source byte");out.player_start_position=std::uint8_t(start);
 const godot::PackedByteArray objective=input.get("objective_bytes",godot::PackedByteArray()),reward=input.get("reward_name_bytes",godot::PackedByteArray());
 if(objective.size()>=512||reward.size()>=32)throw std::invalid_argument("Prepared text exceeds source buffer bounds");
 for(int i=0;i<objective.size();++i)if(!objective[i])throw std::invalid_argument("Prepared objective contains an embedded source terminator");
 for(int i=0;i<reward.size();++i)if(!reward[i])throw std::invalid_argument("Prepared reward contains an embedded source terminator");
 if(objective.size())out.objective.assign(reinterpret_cast<const char*>(objective.ptr()),objective.size());if(reward.size())out.reward_name.assign(reinterpret_cast<const char*>(reward.ptr()),reward.size());
 const godot::PackedByteArray old=input.get("old_state",godot::PackedByteArray());const int sizes[]{1,4,1,12,16,4,4};if(old.size()!=sizes[out.event_type])throw std::invalid_argument("Prepared old-state extent disagrees with original event type");out.old_state.assign(old.ptr(),old.ptr()+old.size());return out;
}
inline godot::PackedByteArray profile_blocks(const ::port::runtime::EventProfilePayload& blocks){
 godot::PackedByteArray out;out.resize(512);for(unsigned tree=0;tree<2;++tree)std::memcpy(out.ptrw()+tree*256,blocks[tree].data(),256);return out;
}
inline ::port::runtime::EventProfilePayload profile_blocks(const godot::PackedByteArray& bytes){
 if(bytes.size()!=512)throw std::invalid_argument("Full event profile transport requires both256-byte blocks");
 ::port::runtime::EventProfilePayload out;for(unsigned tree=0;tree<2;++tree)std::memcpy(out[tree].data(),bytes.ptr()+tree*256,256);return out;
}
inline godot::Dictionary profile_transfer(const ::port::race::EventProfileTransfer& source){
 godot::Dictionary out;out["version"]=int(source.layout.version);godot::PackedInt32Array used;godot::Array records;
 for(int tree=0;tree<2;++tree){used.push_back(int(source.layout.used[tree]));for(unsigned i=0;i<source.layout.trees[tree].size();++i){const auto& entry=source.layout.trees[tree][i];godot::PackedInt32Array row;for(int v:{tree,int(i),entry.event.number,entry.event.type,entry.location.tree_type,int(entry.location.offset),int(entry.size)})row.push_back(v);records.append(row);}}
 out["used"]=used;out["records"]=records;out["blocks"]=profile_blocks(source.blocks);return out;
}
inline ::port::race::EventProfileTransfer profile_transfer(const godot::Dictionary& input){
 if(!input.has("version")||int(input["version"])!=int(::port::runtime::EventProfileVersion::Retail))throw std::invalid_argument("Event profile transport must identify reviewed Retail layout");
 ::port::race::EventProfileTransfer out;out.blocks=profile_blocks(godot::PackedByteArray(input.get("blocks",godot::PackedByteArray())));
 const godot::PackedInt32Array used=input.get("used",godot::PackedInt32Array());const godot::Array records=input.get("records",godot::Array());
 if(used.size()!=2||records.size()!=64)throw std::invalid_argument("Event profile layout requires both trees/all64 record identities");
 bool seen[2][32]{};for(int tree=0;tree<2;++tree){if(used[tree]<0||used[tree]>256)throw std::invalid_argument("Invalid event profile used extent");out.layout.used[tree]=used[tree];out.layout.trees[tree].resize(32);}
 for(int i=0;i<records.size();++i){const godot::PackedInt32Array row=records[i];if(row.size()!=7||row[0]<0||row[0]>1||row[1]<0||row[1]>=32||row[4]<0||row[4]>1||row[5]<0||row[5]>=256||row[6]<1||row[6]>16)throw std::invalid_argument("Invalid event profile semantic record");if(seen[row[0]][row[1]])throw std::invalid_argument("Duplicate event profile record index");seen[row[0]][row[1]]=true;out.layout.trees[row[0]][row[1]]={{row[2],row[3]},{row[4],std::size_t(row[5])},std::size_t(row[6])};}
 // Identity transcoding performs strict type/size/overlap/shared-alias checks.
 (void)::port::runtime::transcode_event_profile(out.layout,out.blocks,out.layout,out.blocks);return out;
}
}
