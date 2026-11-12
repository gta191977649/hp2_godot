#pragma once
// TASK-0104: PC presentation/interaction contract; no PS2 ABI or Godot types.
#include "game_core/indep/src/fmenus.hpp"
#include "port/render/frame.hpp"
#include <string>
#include <vector>
namespace port::runtime {
struct FrontendUiGlyph {
    std::uint16_t code=0;
    render::Rect bounds,source;
};
struct FrontendUiWidget {
    std::string id,kind="widget";
    hp2::frontend::FeWidget widget;
    render::Rect bounds,clip;
    std::vector<FrontendUiGlyph> glyphs; // Native frame-builder visual reference.
    render::Rect text_bounds; // Optional text visual inside a separately sized event hit target.
    bool interactive=false,enabled=true,selected=false,wrap=false;
    int menu_index=-1,stack=0,input_port=0;
    float value=0;
    std::uint32_t handle_texture=0;
};
struct FrontendUiSnapshot {
    std::uint64_t generation=0;
    std::string page;
    bool modal=false,selection=false;
    render::Vec2 display_offset;
    render::Rect event_clip; // Authored event viewport; PC scaling reference only.
    std::vector<FrontendUiWidget> widgets;
};
struct FrontendUiAction {
    enum class Kind {Focus,Activate,Back,Adjust,SetValue,EditText};
    Kind kind=Kind::Activate;
    std::uint64_t generation=0;
    std::string id,text;
    int port=0,direction=0;
    float value=0;
};
}
