#pragma once

#include <memory>

namespace godot {
class InputEvent;
class Node;
template<class T> class Ref;
}

namespace hp2 {

// Godot host diagnostics. All calls run on the main thread.
class ImGuiOverlay {
public:
    explicit ImGuiOverlay(godot::Node& owner);
    ~ImGuiOverlay();
    bool input(const godot::Ref<godot::InputEvent>& event);
    bool captures_game_input() const noexcept;
    void log(const char* event, const char* detail, bool error = false);
    void show_console();
    void frame(double delta);

private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};

}
