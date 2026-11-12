#pragma once
// TASK-0102. Port-only, versioned named-field wire format. Never a Fred ABI
// overlay: each record contains Time32, NameIndex16 and CarType16 separately.
#include "port/runtime/event_launch.hpp"
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <limits>
#include <string>
#include <stdexcept>
namespace port::godot_bridge {
inline godot::PackedInt32Array score_ints(const godot::Dictionary& input,const char* key){
 const godot::Variant value=input.get(key,godot::Variant());
 if(value.get_type()!=godot::Variant::PACKED_INT32_ARRAY)throw std::invalid_argument(std::string("Score transport requires PackedInt32Array: ")+key);
 return value;
}
template<std::size_t N>godot::PackedInt32Array score_records(const std::array<::port::runtime::EventScoreRecord,N>& records){
 godot::PackedInt32Array out;out.resize(N*3);
 for(std::size_t i=0;i<N;++i){out.set(i*3,records[i].time);out.set(i*3+1,records[i].name_index);out.set(i*3+2,records[i].car_type);}return out;
}
template<std::size_t N>void score_records(const godot::PackedInt32Array& input,std::array<::port::runtime::EventScoreRecord,N>& records){
 if(input.size()!=std::int64_t(N*3))throw std::invalid_argument("Score transport record count differs from original axes");
 for(std::size_t i=0;i<N;++i){const auto name=input[i*3+1],car=input[i*3+2];if(name<std::numeric_limits<std::int16_t>::min()||name>std::numeric_limits<std::int16_t>::max()||car<std::numeric_limits<std::int16_t>::min()||car>std::numeric_limits<std::int16_t>::max())throw std::invalid_argument("Score transport loses original 16-bit field representation");records[i]={input[i*3],std::int16_t(name),std::int16_t(car)};}
}
inline godot::Dictionary score_profile(const ::port::runtime::EventScoreProfile& source){
 godot::Dictionary out;out["schema_version"]=1;out["total"]=score_records(source.total);out["lap"]=score_records(source.lap);
 godot::PackedByteArray names;names.resize(128*12);for(unsigned i=0;i<128;++i)for(unsigned j=0;j<12;++j)names.set(i*12+j,source.names[i][j]);out["names"]=names;return out;
}
inline ::port::runtime::EventScoreProfile score_profile(const godot::Dictionary& input){
 if(int(input.get("schema_version",-1))!=1)throw std::invalid_argument("Unknown score named-field schema");
 ::port::runtime::EventScoreProfile out;score_records(score_ints(input,"total"),out.total);score_records(score_ints(input,"lap"),out.lap);
 const godot::Variant value=input.get("names",godot::Variant());if(value.get_type()!=godot::Variant::PACKED_BYTE_ARRAY)throw std::invalid_argument("Score names require original raw slot bytes");
 const godot::PackedByteArray names=value;if(names.size()!=128*12)throw std::invalid_argument("Score names require all128 original12-byte slots");
 for(unsigned i=0;i<128;++i)for(unsigned j=0;j<12;++j)out.names[i][j]=names[i*12+j];return out;
}
inline godot::Dictionary score_transfer(const ::port::race::EventScoreTransfer& source){
 ::port::runtime::validate_event_score_axes(source.axes);godot::Dictionary out;out["schema_version"]=1;out["profile"]=score_profile(source.profile);
 godot::PackedInt32Array laps,tracks;for(auto v:source.axes.lap_counts)laps.push_back(v);for(const auto& track:source.axes.tracks){tracks.push_back(track.number);tracks.push_back(track.same_as);tracks.push_back(track.score_index);}out["lap_counts"]=laps;out["tracks"]=tracks;return out;
}
inline ::port::race::EventScoreTransfer score_transfer(const godot::Dictionary& input){
 if(int(input.get("schema_version",-1))!=1)throw std::invalid_argument("Unknown event score transfer schema");
 ::port::race::EventScoreTransfer out;const auto laps=score_ints(input,"lap_counts"),tracks=score_ints(input,"tracks");
 if(laps.size()!=9||tracks.size()%3||tracks.size()<3||tracks.size()>127*3)throw std::invalid_argument("Event score axis metadata has invalid extent");
 for(unsigned i=0;i<9;++i)out.axes.lap_counts[i]=laps[i];for(std::int64_t i=0;i<tracks.size();i+=3)out.axes.tracks.push_back({tracks[i],tracks[i+1],tracks[i+2]});::port::runtime::validate_event_score_axes(out.axes);
 const godot::Variant profile=input.get("profile",godot::Variant());if(profile.get_type()!=godot::Variant::DICTIONARY)throw std::invalid_argument("Event score transfer omitted its named-field profile");out.profile=score_profile(godot::Dictionary(profile));return out;
}
}
