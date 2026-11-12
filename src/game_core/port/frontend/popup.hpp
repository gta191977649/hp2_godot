#pragma once
// Port semantic storage/services for Debug tPopupMenu, not its PS2 ABI.
// E-0098-X02; reverse/task0098/popup-owner.md. Original code lives in fmenus.cpp.
#include "game_core/indep/src/frontendscreens/selectcar.hpp"
#include <map>

namespace port::frontend {
struct PopupServices {
    std::function<void(hp2::debug::ScreenMessage,hp2::debug::ScreenCallbackData&)> callback;
    std::function<void(int)> sound;
    // Whole original view operations: visibility, three menu items/text/highlight.
    std::function<void()> dismiss_view;
    std::function<void(int)> unhighlight,highlight,set_highlight_animation;
    std::function<std::uint32_t(int)> item_hash;
    std::function<void()> align_arrows;
    std::function<void()> show_view;
    std::function<void(int,std::uint32_t,int)> reset_item;
    std::function<int(int)> render_message;
    std::function<void(std::uint32_t,int)> set_top_y,set_language;
    std::function<void(std::uint32_t)> clear_text;
};
class PopupMenu {
public:
    bool active=false; // Port-owned representation of original activity word.
    selection::Popup config;
    int index=0;
    // Debug0016AAD8; E-0098-X02; explicit original view operations.
    void Sub_0016AAD8(const selection::Popup&,const PopupServices&);
    // Debug0016AE08/0016AF28; E-0098-X02. Decoded services, no ABI claim.
    // Summon/view use their separately evidenced operations below.
    void Sub_0016AE08(int port,int param,const PopupServices&);
    bool Sub_0016AF28(int port,int event,int param,const PopupServices&);
};
// Semantic storage for FlailerScreenList's two original popup slots.
class PopupList {
public:
    std::array<PopupMenu,2> menus;
    // Debug0016C028/0016C080; E-0098-X02. Summon is an explicit whole-entry service.
    int Sub_0016C028(const selection::Popup&,
        const std::function<void(PopupMenu&,const selection::Popup&)>& summon);
    void Sub_0016C080(int id,const PopupServices&);
};
// Port-owned view of original authored resources, never an original object ABI.
// Debug E-0098-X04; item operations in fmenus.cpp, animation in feanim.cpp.
class PopupView {
public:
    hp2::frontend::FeScreen screen;
    PopupView(const hp2::frontend::FeScreen&,port::driving::Memory&,
        hp2::frontend::WidgetMeasure,std::function<std::string(int)>);
    PopupServices services(std::function<void(hp2::debug::ScreenMessage,hp2::debug::ScreenCallbackData&)>,
        std::function<void(int)>);
    const hp2::frontend::FeWidget* ghost_target(const hp2::frontend::FeWidget&)const;
    // Port UI action identity is scoped to the actual owning document/slot.
    std::map<std::uint32_t,std::string> button_targets(const hp2::frontend::FeScreen*,int slot,int count)const;
    void tick(std::uint32_t,float elapsed=0);
private:
    port::driving::Memory& memory_;
    hp2::frontend::WidgetMeasure measure_;
    std::function<std::string(int)> localize_;
    std::array<std::uint32_t,3> state_{{1,1,1}},saved_{};
    std::uint32_t target_=0,timer_=0,now_=0;
    float start_=1,end_=2,arrow_phase_=0;
    void item_state(int,std::uint32_t,bool); // Debug00169000/001690E0.
    void bind_animation(int); // Debug00241FA0.
};
}
