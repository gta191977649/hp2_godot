#pragma once

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <array>
#include <memory>

struct EditorMarkerTexture;

namespace hp2 {
struct FrontendStateView;
struct RaceEffectsFrame;
class ApplicationRuntime;
class RacePresentation;
class GameResources;

class FrontendPresentation {
public:
    FrontendPresentation(godot::Node2D& canvas, const GameResources& files, ApplicationRuntime& runtime,
        RacePresentation& host);
    void pointer_input(const godot::Ref<godot::InputEvent>& event);
    void pointer_button(int index);
    ~FrontendPresentation();
    void update(const FrontendStateView& state, godot::Vector2 viewport,const RaceEffectsFrame& effects);
    void update_rear_view(const godot::Ref<godot::Texture2D>& view,
        const std::array<const EditorMarkerTexture*, 2>& textures, bool ntsc);
    bool showing_loading() const noexcept;
    bool video_playing() const noexcept;
    std::uint64_t video_request() const noexcept;
private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};

}
