#include "event_settings_codec.hpp"
#include "game_core/port/driving/memory.hpp"
#include <cstring>
#include <stdexcept>
namespace port::runtime {
namespace {
struct Layout {std::size_t size,display;};
Layout layout(EventSettingsVersion v){switch(v){case EventSettingsVersion::Retail:return {0x114e0,0x14c};case EventSettingsVersion::Debug:return {0x114dc,0x148};}throw std::invalid_argument("Unknown event settings profile version");}
std::uint32_t word(const std::uint8_t* p){return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|(std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);}
std::int32_t signed_word(const std::uint8_t* p){auto u=word(p);std::int32_t s;std::memcpy(&s,&u,4);return s;}
void put(std::uint8_t* p,std::uint32_t v){for(unsigned j=0;j<4;++j)p[j]=std::uint8_t(v>>(j*8));}
EventPlayerSettings player(const std::uint8_t* p){
 EventPlayerSettings s;s.players_hud_state=p[0];s.rumble=p[1];s.driving_style=p[2];s.transmission_type=p[3];s.drive_camera_pov_index=p[4];s.drive_camera_alternate=p[5];s.split_time_type=p[6];s.preferred_gender=p[7];s.camera_spring=signed_word(p+8);s.lookback_mode=signed_word(p+12);s.preferred_hud_features=word(p+16);
 for(unsigned j=0;j<12;++j)s.player_name[j]=p[20+j];
 s.best_total_time_record=signed_word(p+32);s.best_lap_time_record=signed_word(p+36);s.hud_hotkey_enabled=signed_word(p+40);s.scoring=signed_word(p+44);return s;
}
void store_player(std::uint8_t* p,const EventPlayerSettings& s){
 p[0]=s.players_hud_state;p[1]=s.rumble;p[2]=s.driving_style;p[3]=s.transmission_type;p[4]=s.drive_camera_pov_index;p[5]=s.drive_camera_alternate;p[6]=s.split_time_type;p[7]=s.preferred_gender;put(p+8,std::uint32_t(s.camera_spring));put(p+12,std::uint32_t(s.lookback_mode));put(p+16,s.preferred_hud_features);
 for(unsigned j=0;j<12;++j)p[20+j]=s.player_name[j];
 put(p+32,std::uint32_t(s.best_total_time_record));put(p+36,std::uint32_t(s.best_lap_time_record));put(p+40,std::uint32_t(s.hud_hotkey_enabled));put(p+44,std::uint32_t(s.scoring));
}
EventDisplaySettings display(const std::uint8_t* p){return {p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]};}
void store_display(std::uint8_t* p,const EventDisplaySettings& s){p[0]=s.rear_view_mirror;p[1]=s.units;p[2]=s.one_player_track_map_mode;p[3]=s.two_player_track_map_mode;p[4]=s.be_the_cop_track_map_mode;p[5]=s.leaderboard_mode;p[6]=s.jump_cams;p[7]=s.zone_cams;}
std::uint32_t address(std::uint32_t fred,std::size_t off,std::size_t n){const auto at=std::uint64_t(fred)+off;if(!fred||at+n>0x100000000ull)throw std::invalid_argument("Event settings logical address wraps");return std::uint32_t(at);}
}
EventSettingsTransfer decode_event_settings(EventSettingsVersion v,const std::vector<std::uint8_t>& b){
 const auto l=layout(v);if(b.size()!=l.size)throw std::invalid_argument("Event settings profile has wrong version extent");EventSettingsTransfer out;
 for(unsigned p=0;p<2;++p)out.players[p]=player(b.data()+0x10d7c+p*0x6d0);
 out.display=display(b.data()+l.display);return out;
}
std::vector<std::uint8_t> encode_event_settings(EventSettingsVersion v,const EventSettingsTransfer& s,const std::vector<std::uint8_t>& baseline){
 const auto l=layout(v);if(baseline.size()!=l.size)throw std::invalid_argument("Event settings baseline has wrong version extent");auto out=baseline;
 for(unsigned p=0;p<2;++p)store_player(out.data()+0x10d7c+p*0x6d0,s.players[p]);
 store_display(out.data()+l.display,s.display);return out;
}
EventPlayerSettings read_debug_event_player_settings(const driving::Memory& m,std::uint32_t a){
 const auto at=address(a,0,48);std::array<std::uint8_t,48> b;for(unsigned j=0;j<48;++j)b[j]=m.byte(at+j);return player(b.data());
}
EventSettingsTransfer read_debug_event_settings(const driving::Memory& m,std::uint32_t fred){
 EventSettingsTransfer out;std::array<std::uint8_t,48> bytes;std::array<std::uint8_t,8> d;
 for(unsigned p=0;p<2;++p){const auto a=address(fred,0x10d7c+p*0x6d0,48);for(unsigned j=0;j<48;++j)bytes[j]=m.byte(a+j);out.players[p]=player(bytes.data());}
 const auto at=address(fred,0x148,8);for(unsigned j=0;j<8;++j)d[j]=m.byte(at+j);out.display=display(d.data());return out;
}
void write_debug_event_settings(driving::Memory& m,std::uint32_t fred,const EventSettingsTransfer& s){
 // Preflight all destinations BEFORE named field publication. No subsequent
 // guest lookup/allocation/throwing store occurs in the publication loop.
 auto* p0=m.view(address(fred,0x10d7c,48),48).data;auto* p1=m.view(address(fred,0x1144c,48),48).data;auto* d=m.view(address(fred,0x148,8),8).data;
 store_player(p0,s.players[0]);store_player(p1,s.players[1]);store_display(d,s.display);
}
void set_event_player_name(EventPlayerSettings& s,const std::vector<std::uint8_t>& from){
 unsigned count=0;for(unsigned j=0;j<11;++j){if(j>=from.size())throw std::invalid_argument("Source SetName input ends before NUL/limit11");++count;if(!from[j])break;}
 for(unsigned j=0;j<count;++j)s.player_name[j]=from[j];
}
}
