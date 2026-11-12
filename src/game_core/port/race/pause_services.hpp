#pragma once
// TASK-0112 E-0112-P01. Port services for original screen/device calls,
// not a PS2 MenuSystem/PopupConfig ABI or a replacement pause state machine.
#include "game_core/indep/src/frontendscreens/selectcar.hpp"
namespace port::race {
// Only caller-written PopupConfig operands. DismissParam/PopupID are not
// initialized by these captured callers; no selection-popup defaults are
// imported for them. A full raw popup consumer must resolve those reads.
struct PauseConfirmation {
 std::uint32_t parent_screen,screen_hash,message;
 int port,flags,n_buttons,start_button,cancel_param,title_id;
 std::array<int,3> button_ids,button_params;
 float timeout;
};
struct PauseServices {
 std::function<void(int)> stop_force;
 std::function<void(int,float)> rumble_impact; //Optional physical-device output required on source rumble enable.
 std::function<void(std::uint32_t,std::uint32_t,std::uint32_t,int)> render_objective; //Original rectangle/group/player operands.
 // E-0112-P02. Source ChangeToScreen takes a logical char*, not a hash.
 std::function<void(std::uint32_t,int)> change_screen;
 std::function<void(bool)> mute;
 std::function<int(int)> joystick_type;
 // Screen argument is a source logical char* obtained from PAUSE_* globals.
 std::function<void(std::uint32_t,int)> set_screen_filter_port;
 std::function<bool()> all_joysticks_plugged;
 std::function<void(int)> request_unpause;
 // Original parent screen, source flags/IDs/parameters. Timeout is original0;
 // this callback must bind the existing original PopupMenu lifecycle.
 std::function<std::uint32_t()> current_screen;
 std::function<void(const PauseConfirmation&)> summon_popup;
 std::function<void(std::uint32_t,int,std::uint32_t)> gray_menu_item;
};
}
