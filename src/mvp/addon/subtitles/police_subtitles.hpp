#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

struct SpeechCaptionState;

namespace godot { class Node2D; struct Vector2; }
namespace hp2 {
class GameResources;
class FlailerResources;
struct FlailerRenderManager;

class PoliceSubtitles final {
public:
    PoliceSubtitles(godot::Node2D& canvas, const GameResources& files);
    ~PoliceSubtitles();
    void update(bool enabled, bool racing, bool paused, std::uint64_t world, std::uint64_t replay,
        const SpeechCaptionState& caption, FlailerResources& resources, const FlailerRenderManager& hud,
        const godot::Vector2& viewport);
    const std::string& visible_text() const noexcept;
    bool verified_source() const noexcept;

private:
    struct State;
    std::unique_ptr<State> state_;
};
}
