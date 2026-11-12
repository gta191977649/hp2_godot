#pragma once
// TASK-0102 E-0102-PS02. Port-only named semantic fields; no original host ABI.
// Proven widths/addresses/copy order: settings-profile-audit.md (PS01).
#include <array>
#include <cstdint>
#include <vector>
namespace port::driving {class Memory;}
namespace port::runtime {
enum class EventSettingsVersion { Retail, Debug };
struct EventPlayerSettings {
 std::uint8_t players_hud_state=0,rumble=0,driving_style=0,transmission_type=0;
 std::uint8_t drive_camera_pov_index=0,drive_camera_alternate=0,split_time_type=0,preferred_gender=0;
 std::int32_t camera_spring=0,lookback_mode=0;
 std::uint32_t preferred_hud_features=0;
 std::array<std::uint8_t,12> player_name{};
 std::int32_t best_total_time_record=0,best_lap_time_record=0,hud_hotkey_enabled=0,scoring=0;
};
struct EventDisplaySettings {
 std::uint8_t rear_view_mirror=0,units=0,one_player_track_map_mode=0,two_player_track_map_mode=0;
 std::uint8_t be_the_cop_track_map_mode=0,leaderboard_mode=0,jump_cams=0,zone_cams=0;
};
struct EventSettingsTransfer {
 std::array<EventPlayerSettings,2> players;
 EventDisplaySettings display;
};
// All representable bits survive, including FF char, signed/float-like word
// patterns and dirty/nonterminated name tails. No bool/enum normalization.
EventSettingsTransfer decode_event_settings(EventSettingsVersion,const std::vector<std::uint8_t>&);
std::vector<std::uint8_t> encode_event_settings(EventSettingsVersion,const EventSettingsTransfer&,
 const std::vector<std::uint8_t>& baseline);
EventSettingsTransfer read_debug_event_settings(const driving::Memory&,std::uint32_t fred);
EventPlayerSettings read_debug_event_player_settings(const driving::Memory&,std::uint32_t settings_address);
void write_debug_event_settings(driving::Memory&,std::uint32_t fred,const EventSettingsTransfer&);
// Exact SetName185FB8→bStrNCpy24C9A8 limit11: copies an encountered NUL,
// stops without tail fill, and leaves byte11 untouched even at full length.
// A short nonterminated input is rejected BEFORE changing the destination.
void set_event_player_name(EventPlayerSettings&,const std::vector<std::uint8_t>& source_bytes);
}
