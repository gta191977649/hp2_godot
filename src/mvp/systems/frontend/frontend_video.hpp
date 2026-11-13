#pragma once

#include <godot_cpp/variant/string.hpp>
#include <memory>

namespace godot { class VideoStreamPlayer; }
namespace hp2 {
class GameResources;
class MoviePlayer;

class FrontendVideo {
public:
    FrontendVideo(godot::VideoStreamPlayer& player, const GameResources& assets, const godot::String& ffmpeg);
    ~FrontendVideo();
    void update(MoviePlayer& request);
    bool playing() const noexcept;
    std::uint64_t request_id() const noexcept;
private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};
}
