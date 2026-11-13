#pragma once

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <array>
#include <memory>

struct EditorMarkerTexture;

namespace hp2 {
struct FrontendStateView;
class GameResources;

class FrontendPresentation {
public:
    FrontendPresentation(godot::Node2D& canvas, const GameResources& files);
    ~FrontendPresentation();
    void update(const FrontendStateView& state, godot::Vector2 viewport);
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
