#pragma once

#include <memory>
#include <functional>

namespace godot {
class InputEvent;
class Node;
template<class T> class Ref;
}

namespace hp2 {

struct RaceEffectOptions;
class TrackViewer;

// Godot host diagnostics. All calls run on the main thread.
class ImGuiOverlay {
public:
    explicit ImGuiOverlay(godot::Node& owner);
    ~ImGuiOverlay();
    bool input(const godot::Ref<godot::InputEvent>& event);
    bool captures_game_input() const noexcept;
    bool needs_mouse_cursor() const noexcept;
    const RaceEffectOptions& effect_options() const noexcept;
    void log(const char* event, const char* detail, bool error = false);
    void show_console();
    void attach_track_viewer(TrackViewer& viewer, std::function<void()> open);
    void frame(double delta, bool showFrameGraph);

private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};

}
