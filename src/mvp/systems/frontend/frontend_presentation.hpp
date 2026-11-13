#pragma once

#include <godot_cpp/classes/node2d.hpp>
#include <memory>

namespace hp2 {
struct FrontendStateView;
class GameResources;

class FrontendPresentation {
public:
    FrontendPresentation(godot::Node2D& canvas, const GameResources& files, const godot::String& ffmpeg);
    ~FrontendPresentation();
    void update(const FrontendStateView& state, godot::Vector2 viewport);
    bool video_playing() const noexcept;
    std::uint64_t video_request() const noexcept;
private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};

}
