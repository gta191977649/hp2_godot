#pragma once
// TASK-0112, E-0112-R02. Port-owned device/menu service carriers. All source
// selection and list state remain in original logical Memory, never Godot.
#include "game_core/port/driving/memory.hpp"
#include <functional>
#include "game_core/port/race/pause_services.hpp"
namespace port::race {
struct EndGameScreenServices {
 // Original ChangeToScreen name pointer, root flag, global-vs-menu-list call.
 std::function<void(std::uint32_t,int,bool)> change_screen;
 std::function<void(std::uint32_t,int)> screen_visibility;
 std::function<void()> set_autosave_callback;
 // Exact IsCarUnlocked00244808 child. No decoded car-status substitute.
 std::function<bool(int,int)> is_car_unlocked;
 std::function<void(std::uint32_t,int,std::uint32_t)> gray_menu_item;
 std::function<std::uint32_t()> current_screen;
 std::function<void(const PauseConfirmation&)> summon_popup;
 std::function<void(int,int)> post_message;
 // Optional source speech children; missing sinks guard when source requests.
 std::function<void(int)> unlock_car_speech;
 std::function<void(std::uint32_t)> race_event_speech;
 std::function<void(std::uint32_t,bool,std::uint32_t)> reward_screen_speech;
 std::function<void(int,int,std::uint32_t)> post_lap_record_speech,post_time_record_speech;
 std::function<void(bool,int)> tournament_speech;
 std::function<void()> clear_autosave_popup;
};
// Port owner of original callback page lifetimes. It performs no race settlement.
class ResultSubmenus {
 driving::Memory& memory_;
 std::uint32_t instance_=0,screen_=0;
public:
 explicit ResultSubmenus(driving::Memory& m):memory_(m){}
 void dispatch(std::uint32_t,hp2::debug::ScreenMessage,hp2::debug::ScreenCallbackData&,const EndGameScreenServices&);
 void close();
 std::uint32_t instance()const{return instance_;}
};
void initialize_end_game_presentation(driving::Memory&); //Selective CRT001DE588 data.
// Reviewed Retail-only resource-role adapter, E-0112-R10. Reads source values.
void apply_retail_result_track_header(driving::Memory&,std::uint32_t,int);
}
