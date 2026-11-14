#pragma once

#include "systems/effects/race_effect_options.hpp"

#include <godot_cpp/classes/compositor_effect.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/rd_uniform.hpp>
#include <godot_cpp/classes/render_data.hpp>
#include <array>
#include <mutex>

namespace hp2 {
struct RacePostState;

class RacePostPresentation final : public godot::CompositorEffect {
    GDCLASS(RacePostPresentation, godot::CompositorEffect)
public:
    RacePostPresentation();
    ~RacePostPresentation() override;
    void update(const RacePostState& source, bool reset, const RaceEffectOptions& options);
    void _render_callback(int32_t type, godot::RenderData* data) override;
protected:
    static void _bind_methods();
private:
    void set_test_mode(int mode);
    std::mutex mutex_;
    std::array<float, 32> parameters_{};
    godot::Ref<godot::Texture2D> mask_, filter_, neutral_;
    std::uint32_t maskKey_{}, filterKey_{}, epoch_{}, renderedEpoch_{};
    bool historyValid_{};
    bool radialActive_{};
    bool enabled_{true};
    int testMode_{};
    godot::RID shader_, pipeline_, sampler_, previousColor_;
    std::array<godot::Ref<godot::RDUniform>, 6> bindings_;
    godot::TypedArray<godot::RDUniform> uniforms_;
    godot::PackedByteArray pushConstants_;
};
}
