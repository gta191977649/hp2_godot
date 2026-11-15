#pragma once

#include "systems/effects/race_effect_options.hpp"

#include <godot_cpp/classes/compositor_effect.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/render_data.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/texture2drd.hpp>
#include <array>
#include <atomic>
#include <chrono>

namespace godot { class RenderSceneBuffersRD; }

namespace hp2 {
struct RacePostState;

class RacePostPresentation final : public godot::CompositorEffect {
    GDCLASS(RacePostPresentation, godot::CompositorEffect)
public:
    RacePostPresentation();
    ~RacePostPresentation() override;
    static godot::SubViewport* create_scene(godot::Viewport& output);
    void attach(godot::SubViewport& scene, godot::Viewport& output, godot::Camera3D& camera);
    void detach();
    void stop();
    void update(const RacePostState& source, bool reset, const RaceEffectOptions& options, bool ntsc = true);
    void _render_callback(int32_t type, godot::RenderData* data) override;
protected:
    static void _bind_methods();
private:
    void set_test_mode(int mode);
    void prepare_frame();
    void release_scene_textures();
    void release_render_resources(godot::Ref<RacePostPresentation> keepAlive);
    godot::RID copy_depth(godot::RenderSceneBuffersRD& buffers, const godot::RID& source);
    void update_fog_palette(const std::array<float, 32>& parameters);
    std::array<float, 32> parameters_{};
    godot::Ref<godot::Texture2D> mask_, filter_, neutral_;
    godot::Ref<godot::ImageTexture> fogPalette_;
    std::array<float, 7> fogPaletteKey_{};
    std::array<float, 256 * 4> fogPaletteEntries_{};
    bool baselineFog_{};
    bool profileOptimizedFog_{};
    std::uint32_t maskKey_{}, filterKey_{};
    bool historyValid_{};
    bool historyEnabled_{};
    std::uint32_t historyIndex_{};
    std::chrono::steady_clock::time_point lastHistoryFrame_{};
    bool active_{};
    bool connected_{};
    int testMode_{};
    godot::SubViewport *scene_{}, *focusViewport_{};
    godot::Viewport* output_{};
    godot::Camera3D* camera_{};
    godot::Node* display_{};
    godot::ColorRect* displayRect_{};
    std::array<godot::SubViewport*, 2> historyViewports_{};
    godot::Ref<godot::ShaderMaterial> focusPass_, displayPass_, directFogPass_;
    godot::Ref<godot::Shader> historyShader_, seedShader_, fixtureShader_;
    std::array<godot::Ref<godot::ShaderMaterial>, 2> historyPasses_;
    godot::Ref<godot::Texture2DRD> color_, depth_;
    godot::RID colorRid_, depthRid_;
    godot::RID depthCopyShader_, depthCopyPipeline_, depthCopySampler_;
    bool fxaa_{};
    godot::String aaMode_;
    bool unsupportedDepthReported_{};
    godot::Vector2i extent_;
    std::atomic<bool> sceneTexturesReady_{};
};
}
