#pragma once
// Port-only ImGui presentation; no original counterpart or simulation ownership.
#include <godot_cpp/variant/dictionary.hpp>
#include <deque>
#include <cstdint>
#include <array>
namespace port::godot_bridge {
class ProfileDebugUi {
    std::deque<godot::Dictionary> history_;
    // CPU4, GPU5, counters2. Only populated by the independently flagged path.
    std::deque<std::array<float,11>> graph_history_;
    bool compact_history_active_=false;
    bool paused_=false;
    int category_=0,selected_=0;
    std::int64_t frame_=-1;
public:
    godot::Dictionary draw(const godot::Dictionary&,float width,float height);
};
}
