#pragma once
// TASK-0102 PS02. Port-only named-field schema; no original settings ABI copy.
#include "port/runtime/event_launch.hpp"
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <limits>
#include <stdexcept>
namespace port::godot_bridge {
inline godot::Dictionary settings_transfer(const ::port::runtime::EventSettingsTransfer& source){
 godot::Dictionary out;out["schema_version"]=1;godot::Array players;
 for(const auto& p:source.players){
  godot::Dictionary row;godot::PackedInt64Array fields;
  const std::int64_t values[]{p.players_hud_state,p.rumble,p.driving_style,p.transmission_type,p.drive_camera_pov_index,p.drive_camera_alternate,p.split_time_type,p.preferred_gender,p.camera_spring,p.lookback_mode,p.preferred_hud_features,p.best_total_time_record,p.best_lap_time_record,p.hud_hotkey_enabled,p.scoring};
  for(auto value:values)fields.push_back(value);row["fields"]=fields;
  godot::PackedByteArray name;for(auto byte:p.player_name)name.push_back(byte);row["player_name"]=name;players.append(row);
 }
 out["players"]=players;const auto& d=source.display;godot::PackedByteArray display;
 const std::uint8_t values[]{d.rear_view_mirror,d.units,d.one_player_track_map_mode,d.two_player_track_map_mode,d.be_the_cop_track_map_mode,d.leaderboard_mode,d.jump_cams,d.zone_cams};
 for(auto value:values)display.push_back(value);out["display"]=display;return out;
}
inline ::port::runtime::EventSettingsTransfer settings_transfer(const godot::Dictionary& input){
 if(int(input.get("schema_version",-1))!=1)throw std::invalid_argument("Unknown named event settings schema");
 const godot::Variant rows=input.get("players",godot::Variant());if(rows.get_type()!=godot::Variant::ARRAY)throw std::invalid_argument("Event settings require both named player records");
 const godot::Array players=rows;if(players.size()!=2)throw std::invalid_argument("Event settings player count differs from original extent");
 ::port::runtime::EventSettingsTransfer out;
 for(unsigned index=0;index<2;++index){
  const godot::Variant item=players[index];if(item.get_type()!=godot::Variant::DICTIONARY)throw std::invalid_argument("Event player settings are not a named record");const godot::Dictionary row=item;
  const godot::Variant raw=row.get("fields",godot::Variant());if(raw.get_type()!=godot::Variant::PACKED_INT64_ARRAY)throw std::invalid_argument("Event player settings require lossless numeric fields");const godot::PackedInt64Array f=raw;
  if(f.size()!=15)throw std::invalid_argument("Event player settings omitted named numeric fields");
  for(unsigned i=0;i<15;++i){const auto value=f[i];if(i<8){if(value<0||value>255)throw std::invalid_argument("Event player byte loses original bits");}else if(i==10){if(value<0||std::uint64_t(value)>0xffffffffull)throw std::invalid_argument("Event HUD mask loses original uint32 bits");}else if(value<std::numeric_limits<std::int32_t>::min()||value>std::numeric_limits<std::int32_t>::max())throw std::invalid_argument("Event player word loses original int32 bits");}
  const godot::Variant bytes=row.get("player_name",godot::Variant());if(bytes.get_type()!=godot::Variant::PACKED_BYTE_ARRAY)throw std::invalid_argument("Event player name requires original raw bytes");const godot::PackedByteArray name=bytes;if(name.size()!=12)throw std::invalid_argument("Event player name has invalid source extent");
  auto& p=out.players[index];p.players_hud_state=f[0];p.rumble=f[1];p.driving_style=f[2];p.transmission_type=f[3];p.drive_camera_pov_index=f[4];p.drive_camera_alternate=f[5];p.split_time_type=f[6];p.preferred_gender=f[7];p.camera_spring=f[8];p.lookback_mode=f[9];p.preferred_hud_features=f[10];p.best_total_time_record=f[11];p.best_lap_time_record=f[12];p.hud_hotkey_enabled=f[13];p.scoring=f[14];for(unsigned i=0;i<12;++i)p.player_name[i]=name[i];
 }
 const godot::Variant bytes=input.get("display",godot::Variant());if(bytes.get_type()!=godot::Variant::PACKED_BYTE_ARRAY)throw std::invalid_argument("Event display settings require named source bytes");const godot::PackedByteArray d=bytes;if(d.size()!=8)throw std::invalid_argument("Event display settings have invalid extent");
 out.display={d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7]};return out;
}
}
