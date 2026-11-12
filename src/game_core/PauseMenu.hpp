// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class PauseMenu {
public:
    // No member address is confirmed; no speculative members are declared.
};
} // namespace hp2::recovered

#include "game_core/port/driving/object_view.hpp"
namespace port::race {struct PauseServices;}
#include "game_core/indep/src/frontendscreens/selectcar.hpp"
namespace hp2::debug {
// Original enumerator spellings/values captured read-only, E-0112-P01.
enum ePauseMenuState {ePAUSE_NOT_PAUSED=0,ePAUSE_NORMAL=1,
 ePAUSE_WAIT_FOR_RESTART_CONFIRMATION=2,ePAUSE_WAIT_FOR_QUIT_CONFIRMATION=3};
// E-0099-W62, original frontendscreens/pausemenu.cpp, Debug0016F660.
class PauseMenu:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 void Sub_0016F660();void Sub_0016F6C0(int flags);
 void EnterPause(int,const ::port::race::PauseServices&); //0016F718.
 void ExitPause(); //0016F7F8.
 bool MaybeResume(int,const ::port::race::PauseServices&); //0016FC80.
 void MaybeRestart(const ::port::race::PauseServices&); //0016FA30.
 void MaybeQuit(const ::port::race::PauseServices&); //0016FB58.
 void Activate(std::uint32_t,const ::port::race::PauseServices&); //0016F810.
 void DeActivate(); //0016F9A8, original empty.
 void CheckJoysticks(); //0016FA28, original empty.
 void MenuCallback(ScreenMessage,ScreenCallbackData&,const ::port::race::PauseServices&); //0016FCD8.
 void PauseMenuCallback(ScreenMessage,ScreenCallbackData&,const ::port::race::PauseServices&); //00170070 wrapper.
};
}

namespace hp2::debug {int HandlePauseMenuReplaySpecialEvents(::port::driving::Memory&,std::uint32_t info); /*0016F4D8,E-0099-W81*/}

namespace hp2::debug {
void SetStormType(int); //0016F4B8, instruction-proven empty.
int GetStormType(); //0016F4C0, original literal1.
std::uint32_t GetStormTypeString(int); //0016F4C8, original guest literal.
void SwitchRearViewMirrorFromPauseMenu(::port::driving::Memory&,int); //001803A0.
}
