#pragma once
// Port-side declaration/service header; no original header-path or object ABI claim.
#include "game_core/indep/src/frontendscreens/selectcar.hpp"

namespace port::selection {
// TASK-0098 E-0098-E01. Required semantic bindings of original globals/leaf;
// these callbacks do not claim a production page or original pointer ABI.
struct EventEntryServices {
    hp2::debug::FrontendDatabase& fred;
    ::port::driving::init::ByteView race;
    int& challenge_race_mode;
    const std::array<hp2::debug::CarTypeInfo,64>& cars;
    int language=0;
    std::function<void(hp2::debug::EventTree&,const hp2::debug::EventInfo&)> set_race;
    std::function<void()> build_unlock_list;
    std::function<hp2::debug::eEventStatus(const hp2::debug::EventInfo&)> get_status;
    std::function<void(const char*,bool)> change_screen;
    std::function<void(const char*)> push_screen;
    std::function<void()> continue_event;
};
}
namespace hp2::debug {
// Decoded original fields only. No original allocation/layout/rendering claim.
struct tTreeDetails {
    EventTree* pTree=nullptr;
    const EventInfo* LastKnownEvent=nullptr;
    // Debug001F2BD8/001F2D20, E-0098-E01; explicit host leaf/global services.
    void StartEvent(ScreenCallbackData&,const ::port::selection::EventEntryServices&);
    void HandleMessages(ScreenMessage,ScreenCallbackData&,const ::port::selection::EventEntryServices&);
};
}

