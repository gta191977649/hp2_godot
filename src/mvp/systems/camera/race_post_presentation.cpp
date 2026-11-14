#include "race_post_presentation.hpp"

#include "assets/shader_resource.hpp"
#include "indep/inc/Flags.hpp"
#include "indep/inc/Texture.hpp"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/compositor.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/render_data.hpp>
#include <godot_cpp/classes/rd_texture_format.hpp>
#include <godot_cpp/classes/rd_sampler_state.hpp>
#include <godot_cpp/classes/rd_shader_source.hpp>
#include <godot_cpp/classes/rd_shader_spirv.hpp>
#include <godot_cpp/classes/rd_uniform.hpp>
#include <godot_cpp/classes/render_scene_buffers_rd.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/uniform_set_cache_rd.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace hp2 {
using namespace godot;
namespace {
bool race_fxaa_enabled() {
    return ProjectSettings::get_singleton()->get_setting("hp2/race_fxaa", false);
}

Ref<Texture2D> upload(const EditorMarkerTexture* descriptor) {
    const auto* source = textureInfoForDescriptor(descriptor);
    if (!source || !source->pixels.Data()) return {};
    std::uint32_t w{}, h{};
    const auto rgba = DecodeTextureMipRgba(*source, 0, 0, source->pixels.size, w, h);
    PackedByteArray bytes;
    bytes.resize(rgba.size());
    std::memcpy(bytes.ptrw(), rgba.data(), rgba.size());
    return ImageTexture::create_from_image(Image::create_from_data(w, h, false, Image::FORMAT_RGBA8, bytes));
}
}

RacePostPresentation::RacePostPresentation() {
    fxaa_ = race_fxaa_enabled();
    const auto* os = OS::get_singleton();
    const auto mode = os->get_environment("HP2_MVP_PROFILE_FOG");
    if (!os->get_environment("HP2_PROFILE_DIR").is_empty()) {
        baselineFog_ = mode == "baseline" || mode == "no-fog" || mode == "no-focus" || mode == "no-history";
        profileOptimizedFog_ = mode == "optimized";
    }
    set_effect_callback_type(EFFECT_CALLBACK_TYPE_POST_TRANSPARENT);
    set_access_resolved_color(true);
    set_access_resolved_depth(true);
    auto image = Image::create_empty(1, 1, false, Image::FORMAT_RGBA8);
    image->fill(Color(1,1,1,1));
    neutral_ = ImageTexture::create_from_image(image);
}

void RacePostPresentation::update_fog_palette(const std::array<float, 32>& p) {
    if (p[11] < 0.0f) return;
    const std::array key{p[0], p[1], p[2], p[3], p[8], p[9], p[11]};
    if (fogPalette_.is_valid() && key == fogPaletteKey_) return;
    const float denominator = std::max(1.0f, 255.0f - p[8]);
    std::array<float, 256 * 4> entries{};
    for (int slot = 0; slot < 256; ++slot) {
        const float density = std::clamp((255.0f - slot - p[8]) / denominator, 0.0f, 1.0f);
        auto* entry = entries.data() + slot * 4;
        std::copy_n(p.data(), 3, entry);
        switch (static_cast<int>(p[11])) {
        case 1:
            std::fill_n(entry, 3, 0.0f);
            entry[3] = (1.0f - density) * p[3];
            break;
        case 2:
            std::fill_n(entry, 3, slot / 255.0f);
            entry[3] = 1.0f;
            break;
        case 3:
            entry[3] = float(slot) == std::trunc(p[8]) ? p[3] : 0.0f;
            break;
        default:
            entry[3] = std::floor(std::pow(std::max(0.01f, density), p[9]) * p[3] * 128.0f) / 128.0f;
            break;
        }
    }
    fogPaletteKey_ = key;
    if (fogPalette_.is_valid() && entries == fogPaletteEntries_) return;
    fogPaletteEntries_ = entries;
    PackedByteArray bytes;
    bytes.resize(sizeof(entries));
    std::memcpy(bytes.ptrw(), entries.data(), sizeof(entries));
    const auto image = Image::create_from_data(256, 1, false, Image::FORMAT_RGBAF, bytes);
    if (fogPalette_.is_valid()) {
        fogPalette_->update(image);
    } else {
        fogPalette_ = ImageTexture::create_from_image(image);
        for (const auto& pass : {directFogPass_, focusPass_, historyPasses_[0], historyPasses_[1]})
            pass->set_shader_parameter("fog_palette", fogPalette_);
    }
}

void RacePostPresentation::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_test_mode", "mode"), &RacePostPresentation::set_test_mode);
}

void RacePostPresentation::set_test_mode(int mode) {
    if (OS::get_singleton()->get_environment("HP2_MVP_TEST_POST") != "1") return;
    if (!(testMode_ == 6 && (mode == 7 || mode == 8))) historyValid_ = false;
    testMode_ = mode;
    set_enabled(true);
}

void RacePostPresentation::update(const RacePostState& s, bool reset, const RaceEffectOptions& options, bool ntsc) {
    const auto maskKey = s.radialMask ? s.radialMask->key : 0;
    const auto filterKey = s.filterTexture ? s.filterTexture->key : 0;
    if (maskKey != maskKey_) {
        mask_ = s.radialMask ? upload(s.radialMask) : Ref<Texture2D>();
        maskKey_ = maskKey;
        reset = true;
    }
    if (filterKey != filterKey_) {
        filter_ = s.filterTexture ? upload(s.filterTexture) : Ref<Texture2D>();
        filterKey_ = filterKey;
    }
    const bool history = options.motionBlur && mask_.is_valid();
    const bool radial = history && s.radialActive;
    const bool fog = options.fog && s.fogEnabled && (s.fog[3] > 0.0f || s.fogMode != 0);
    const bool monochrome = options.monochrome && s.monochrome;
    const float filterAlpha = options.colorFilter && filter_.is_valid() ? s.filterAlpha / 128.0f : 0.0f;
    const int focusPasses = options.depthOfField && s.focusEnabled ? std::clamp(s.focusPasses, 0, 6) : 0;
    reset |= history != historyEnabled_;
    historyEnabled_ = history;
    if (reset) historyValid_ = false;
    parameters_ = {s.fog[0]/255, s.fog[1]/255, s.fog[2]/255, s.fog[3],
        s.focus[0], s.focus[1], s.focus[2]/128, s.focus[3]/128,
        s.fogRange[0], s.fogPower, s.fogRange[1], fog ? float(s.fogMode) : -1.0f,
        s.filterColor[0]/128.0f, s.filterColor[1]/128.0f, s.filterColor[2]/128.0f, 0,
        float(s.radialAmounts[0]), float(s.radialAmounts[1]), filterAlpha,
        monochrome ? 1.0f : 0.0f, 0, 0, radial ? 1.0f : 0.0f, 0,
        float(s.focusTransition), float(focusPasses),
        s.focusJitter/128.0f, 0, history ? 1.0f : 0.0f, ntsc ? 448.0f : 512.0f};
    active_ = fog || focusPasses > 0 || monochrome || history || filterAlpha > 0.0f;
}

namespace {
Ref<ShaderMaterial> full_screen_pass(Node& parent, const char* path, ColorRect** output = nullptr) {
    auto* rect = memnew(ColorRect);
    rect->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
    parent.add_child(rect);
    rect->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
    Ref<ShaderMaterial> material;
    material.instantiate();
    material->set_shader(load_shader(path));
    rect->set_material(material);
    if (output) *output = rect;
    return material;
}

SubViewport* canvas_target(Viewport& output, const char* name) {
    auto* view = memnew(SubViewport);
    view->set_name(name);
    view->set_disable_3d(true);
    view->set_use_hdr_2d(true);
    view->set_update_mode(SubViewport::UPDATE_DISABLED);
    output.add_child(view);
    return view;
}
}

SubViewport* RacePostPresentation::create_scene(Viewport& output) {
    auto* scene = memnew(SubViewport);
    scene->set_name("RaceRenderTarget");
    scene->set_size(output.get_visible_rect().size);
    // With MSAA, Forward+ exposes R32 resolved depth. The FXAA path copies
    // the native depth into an R32 texture before the canvas samples it.
    scene->set_msaa_3d(race_fxaa_enabled() ? Viewport::MSAA_DISABLED : Viewport::MSAA_2X);
    scene->set_update_mode(SubViewport::UPDATE_ALWAYS);
    output.add_child(scene);
    return scene;
}

void RacePostPresentation::attach(SubViewport& scene, Viewport& output, Camera3D& camera) {
    scene_ = &scene;
    output_ = &output;
    camera_ = &camera;
    color_.instantiate();
    depth_.instantiate();
    // Reserve stable RenderingServer handles before the materials reference
    // them. The render callback replaces the backing textures in place.
    const auto initialize = callable_mp(this, &RacePostPresentation::release_scene_textures);
    RenderingServer::get_singleton()->call_on_render_thread(initialize);
    focusViewport_ = canvas_target(output, "RaceFocusTarget");
    focusPass_ = full_screen_pass(*focusViewport_, "res://shader/race_post_focus.gdshader");
    historyShader_ = load_shader("res://shader/race_post_history.gdshader");
    for (std::size_t i = 0; i < historyViewports_.size(); ++i) {
        historyViewports_[i] = canvas_target(output, i ? "RaceHistoryB" : "RaceHistoryA");
        auto& pass = historyPasses_[i];
        pass = full_screen_pass(*historyViewports_[i], "res://shader/race_post_history.gdshader");
        pass->set_shader_parameter("focus_image", focusViewport_->get_texture());
    }
    auto* layer = memnew(CanvasLayer);
    layer->set_layer(-2);
    output.add_child(layer);
    display_ = layer;
    displayPass_ = full_screen_pass(*layer, "res://shader/race_post_output.gdshader", &displayRect_);
    displayPass_->set_shader_parameter("fallback_scene", scene_->get_texture());
    displayPass_->set_shader_parameter("fallback_fxaa_scene", scene_->get_texture());
    displayPass_->set_shader_parameter("post_fxaa_color", neutral_);
    displayPass_->set_shader_parameter("fxaa", fxaa_);
    directFogPass_.instantiate();
    directFogPass_->set_shader(load_shader("res://shader/race_post_fog.gdshader"));
    for (const auto& pass : {directFogPass_, focusPass_, historyPasses_[0], historyPasses_[1]}) {
        pass->set_shader_parameter("scene_color", color_);
        pass->set_shader_parameter("scene_depth", depth_);
        pass->set_shader_parameter("use_fog_palette", pass == directFogPass_ || profileOptimizedFog_);
        pass->set_shader_parameter("fog_palette", neutral_);
    }
    Ref<Compositor> compositor;
    compositor.instantiate();
    TypedArray<CompositorEffect> passes;
    passes.push_back(Ref<RacePostPresentation>(this));
    compositor->set_compositor_effects(passes);
    camera.set_compositor(compositor);
    RenderingServer::get_singleton()->connect("frame_pre_draw", callable_mp(this, &RacePostPresentation::prepare_frame));
    connected_ = true;
    prepare_frame();
}

RacePostPresentation::~RacePostPresentation() {
    if (connected_) RenderingServer::get_singleton()->disconnect("frame_pre_draw",
        callable_mp(this, &RacePostPresentation::prepare_frame));
}

void RacePostPresentation::stop() {
    if (!connected_) return;
    RenderingServer::get_singleton()->disconnect("frame_pre_draw",
        callable_mp(this, &RacePostPresentation::prepare_frame));
    connected_ = false;
    RenderingServer::get_singleton()->call_on_render_thread(callable_mp(this,
        &RacePostPresentation::release_render_resources));
}

void RacePostPresentation::detach() {
    if (!scene_) return;
    stop();
    camera_->set_compositor({});
    for (auto* node : {static_cast<Node*>(focusViewport_), static_cast<Node*>(historyViewports_[0]),
                       static_cast<Node*>(historyViewports_[1]), display_}) {
        output_->remove_child(node);
        node->queue_free();
    }
    scene_ = nullptr;
}

void RacePostPresentation::prepare_frame() {
    const Vector2i size = output_->get_visible_rect().size;
    if (size.x < 2 || size.y < 2) return;
    if (size != extent_) {
        // Reset the RenderingServer wrappers before resizing invalidates the
        // borrowed RenderingDevice textures and their cached canvas bindings.
        sceneTexturesReady_ = false;
        displayPass_->set_shader_parameter("post_color", neutral_);
        if (fxaa_) displayPass_->set_shader_parameter("post_fxaa_color", neutral_);
        for (const auto& pass : historyPasses_) {
            pass->set_shader_parameter("previous_image", neutral_);
            pass->set_shader_parameter("focus_image", neutral_);
        }
        RenderingServer::get_singleton()->call_on_render_thread(callable_mp(this,
            &RacePostPresentation::release_scene_textures));
        scene_->set_size(size);
        focusViewport_->set_size({(size.x + 1) / 2, (size.y + 1) / 2});
        for (auto* target : historyViewports_) target->set_size(size);
        extent_ = size;
        historyValid_ = false;
    }

    auto p = parameters_;
    if (testMode_ != 0) {
        p[11] = -1;
        p[18] = p[19] = p[22] = p[25] = p[28] = 0;
        if (testMode_ == 1) p[19] = 1;
        if (testMode_ == 2) { p[4] = 20; p[5] = 2; p[6] = 0; p[7] = 1; p[25] = 2; }
        if (testMode_ == 3 || (testMode_ >= 5 && testMode_ <= 8)) {
            p[16] = 0; p[17] = 204;
            p[28] = 1;
            p[31] = float(testMode_);
            p[22] = testMode_ == 5 || testMode_ == 6 ? 0 : 1;
        }
        if (testMode_ == 4) { p[12] = 0.8f; p[13] = 0.3f; p[14] = 0.2f; p[18] = 0.7f; }
        if (testMode_ >= 9 && testMode_ <= 11) {
            p[8] = 0;
            p[10] = 16777215;
            p[11] = 2;
            p[22] = p[28] = p[31] = 0;
        }
    }

    const bool draw = get_enabled() && (active_ || testMode_ != 0) &&
        (sceneTexturesReady_ || testMode_ == 6);
    const bool directFog = draw && !fxaa_ && !baselineFog_ && (testMode_ == 0 || testMode_ == 10) && p[11] >= 0.0f &&
        p[18] == 0 && p[19] == 0 && p[25] == 0 && p[28] == 0;
    const auto next = historyIndex_ ^ 1u;
    for (auto* target : historyViewports_) target->set_update_mode(SubViewport::UPDATE_DISABLED);
    focusViewport_->set_update_mode(draw && p[25] > 0 ? SubViewport::UPDATE_ALWAYS : SubViewport::UPDATE_DISABLED);
    displayPass_->set_shader_parameter("use_post", draw);
    displayPass_->set_shader_parameter("seed_history", testMode_ == 6);
    const auto& display = directFog ? directFogPass_ : displayPass_;
    if (displayRect_->get_material() != display) displayRect_->set_material(display);
    if (!draw) {
        historyValid_ = false;
        return;
    }
    // Prefer the legacy combined pass for the normal flyby. It averaged
    // more FPS in the paired runs. The fog-only path avoids a full pass.
    if (directFog || profileOptimizedFog_) update_fog_palette(p);
    if (directFog) {
        directFogPass_->set_shader_parameter("fog_params", Vector4(p[8], p[9], p[10], p[11]));
        historyValid_ = false;
        return;
    }

    p[20] = camera_->get_near();
    p[21] = camera_->get_far();
    const auto now = std::chrono::steady_clock::now();
    const float referenceHz = p[29] == 448.0f ? 60.0f : 50.0f;
    const float sourceFrames = std::chrono::duration<float>(now - lastHistoryFrame_).count() * referenceHz;
    if (p[31] == 0 && sourceFrames > 6.0f) historyValid_ = false;
    const bool sourceCadence = p[31] != 0 || std::abs(sourceFrames - 1.0f) < 0.05f;
    p[30] = p[31] == 8 ? 0.5f : (sourceCadence ? 1.0f : std::max(sourceFrames, 0.001f));
    if (!historyValid_) p[22] = 0;
    lastHistoryFrame_ = now;
    historyValid_ = p[28] > 0;
    if (p[31] < 7.0f) historyIndex_ = next;
    constexpr std::array names{"fog", "focus", "fog_params", "filter_color", "effects", "camera", "kernel", "history"};
    const auto& pass = historyPasses_[next];
    if (testMode_ == 6 && seedShader_.is_null()) seedShader_ = load_shader("res://shader/race_post_history_seed.gdshader");
    if (testMode_ >= 7 && testMode_ <= 8 && fixtureShader_.is_null())
        fixtureShader_ = load_shader("res://shader/race_post_history_fixture.gdshader");
    const auto& shader = testMode_ == 6 ? seedShader_ :
        testMode_ >= 7 && testMode_ <= 8 ? fixtureShader_ : historyShader_;
    if (pass->get_shader() != shader) {
        pass->set_shader(shader);
        pass->set_shader_parameter("scene_color", color_);
        pass->set_shader_parameter("scene_depth", depth_);
        const Ref<Texture2D> palette = fogPalette_.is_valid() ? Ref<Texture2D>(fogPalette_) : neutral_;
        pass->set_shader_parameter("fog_palette", palette);
    }
    pass->set_shader_parameter("use_fog_palette", profileOptimizedFog_ && testMode_ != 11);
    Ref<Texture2D> previous = neutral_;
    if (p[22] > 0) previous = historyViewports_[next ^ 1u]->get_texture();
    pass->set_shader_parameter("previous_image", previous);
    Ref<Texture2D> focus = neutral_;
    if (p[25] > 0) focus = focusViewport_->get_texture();
    pass->set_shader_parameter("focus_image", focus);
    for (std::size_t i = 0; i < names.size(); ++i) {
        const auto* v = p.data() + 4 * i;
        const Vector4 value(v[0], v[1], v[2], v[3]);
        pass->set_shader_parameter(names[i], value);
        if (p[25] > 0) focusPass_->set_shader_parameter(names[i], value);
    }
    pass->set_shader_parameter("radial_mask", mask_.is_valid() ? mask_ : neutral_);
    pass->set_shader_parameter("filter_image", filter_.is_valid() ? filter_ : neutral_);
    displayPass_->set_shader_parameter("post_color", historyViewports_[next]->get_texture());
    if (fxaa_) displayPass_->set_shader_parameter("post_fxaa_color", historyViewports_[next]->get_texture());
    historyViewports_[next]->set_update_mode(SubViewport::UPDATE_ALWAYS);
}

void RacePostPresentation::_render_callback(int32_t type, RenderData* data) {
    if (type != EFFECT_CALLBACK_TYPE_POST_TRANSPARENT || !data) return;
    const Ref<RenderSceneBuffersRD> buffers = data->get_render_scene_buffers();
    if (buffers.is_null() || buffers->get_view_count() != 1) return;
    // Godot owns these textures. Refresh the resource wrappers when its render
    // buffers change. The dependent canvas viewports sample them after this pass.
    const auto color = buffers->get_color_texture();
    auto depth = buffers->get_depth_texture();
    if (fxaa_ && depth.is_valid()) depth = copy_depth(*buffers.ptr(), depth);
    if (!depth.is_valid()) {
        sceneTexturesReady_ = false;
        return;
    }
    if (color.is_valid() && color != colorRid_) {
        color_->set_texture_rd_rid(color);
        colorRid_ = color;
    }
    if (depth.is_valid() && depth != depthRid_) {
        const auto format = RenderingServer::get_singleton()->get_rendering_device()->texture_get_format(depth);
        if (format->get_format() != RenderingDevice::DATA_FORMAT_R32_SFLOAT) {
            if (!unsupportedDepthReported_) {
                UtilityFunctions::push_error("HP2 race post requires Forward+ resolved R32 depth.");
                unsupportedDepthReported_ = true;
            }
            return;
        }
        depth_->set_texture_rd_rid(depth);
        depthRid_ = depth;
    }
    sceneTexturesReady_ = colorRid_.is_valid() && depthRid_.is_valid();
}

RID RacePostPresentation::copy_depth(RenderSceneBuffersRD& buffers, const RID& source) {
    using RD = RenderingDevice;
    auto* rd = RenderingServer::get_singleton()->get_rendering_device();
    if (!depthCopyPipeline_.is_valid()) {
        Ref<RDShaderSource> code;
        code.instantiate();
        code->set_stage_source(RD::SHADER_STAGE_COMPUTE, R"glsl(#version 450
layout(local_size_x=16, local_size_y=16) in;
layout(set=0, binding=0) uniform sampler2D native_depth;
layout(r32f, set=0, binding=1) uniform writeonly image2D color_depth;
void main() {
    ivec2 xy = ivec2(gl_GlobalInvocationID.xy);
    if (any(greaterThanEqual(xy, imageSize(color_depth)))) return;
    imageStore(color_depth, xy, vec4(texelFetch(native_depth, xy, 0).r));
}
)glsl");
        const auto spirv = rd->shader_compile_spirv_from_source(code);
        if (spirv.is_null()) return {};
        depthCopyShader_ = rd->shader_create_from_spirv(spirv, "HP2 single-sample depth");
        if (!depthCopyShader_.is_valid()) return {};
        depthCopyPipeline_ = rd->compute_pipeline_create(depthCopyShader_);
        Ref<RDSamplerState> sampler;
        sampler.instantiate();
        depthCopySampler_ = rd->sampler_create(sampler);
    }
    const auto size = buffers.get_internal_size();
    const auto target = buffers.has_texture("HP2RacePost", "DepthCopy")
        ? buffers.get_texture("HP2RacePost", "DepthCopy")
        : buffers.create_texture("HP2RacePost", "DepthCopy", RD::DATA_FORMAT_R32_SFLOAT,
            RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT, RD::TEXTURE_SAMPLES_1,
            size, 1, 1, true, false);
    if (!target.is_valid()) return {};
    Ref<RDUniform> input, output;
    input.instantiate();
    input->set_uniform_type(RD::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE);
    input->add_id(depthCopySampler_);
    input->add_id(source);
    output.instantiate();
    output->set_uniform_type(RD::UNIFORM_TYPE_IMAGE);
    output->set_binding(1);
    output->add_id(target);
    TypedArray<RDUniform> uniforms;
    uniforms.push_back(input);
    uniforms.push_back(output);
    const auto set = UniformSetCacheRD::get_cache(depthCopyShader_, 0, uniforms);
    if (!set.is_valid()) return {};
    const auto list = rd->compute_list_begin();
    rd->compute_list_bind_compute_pipeline(list, depthCopyPipeline_);
    rd->compute_list_bind_uniform_set(list, set, 0);
    rd->compute_list_dispatch(list, (size.x+15)/16, (size.y+15)/16, 1);
    rd->compute_list_end();
    return target;
}

void RacePostPresentation::release_render_resources() {
    release_scene_textures();
    auto* rd = RenderingServer::get_singleton()->get_rendering_device();
    for (auto* resource : {&depthCopyPipeline_, &depthCopyShader_, &depthCopySampler_}) {
        if (resource->is_valid()) rd->free_rid(*resource);
        *resource = {};
    }
}

void RacePostPresentation::release_scene_textures() {
    // Keep the RenderingServer handles used by the canvas materials.
    auto* server = RenderingServer::get_singleton();
    const auto neutral = server->texture_get_rd_texture(neutral_->get_rid());
    color_->set_texture_rd_rid(neutral);
    depth_->set_texture_rd_rid(neutral);
    colorRid_ = {};
    depthRid_ = {};
}
}
