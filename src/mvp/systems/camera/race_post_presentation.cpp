#include "race_post_presentation.hpp"

#include "indep/inc/Flags.hpp"
#include "indep/inc/Texture.hpp"

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/rd_sampler_state.hpp>
#include <godot_cpp/classes/rd_shader_source.hpp>
#include <godot_cpp/classes/rd_shader_spirv.hpp>
#include <godot_cpp/classes/rd_uniform.hpp>
#include <godot_cpp/classes/render_data.hpp>
#include <godot_cpp/classes/render_scene_buffers_rd.hpp>
#include <godot_cpp/classes/render_scene_data.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/uniform_set_cache_rd.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <cstring>

namespace hp2 {
using namespace godot;
namespace {
constexpr auto shaderCode = R"(
#version 450
layout(local_size_x=8, local_size_y=8) in;
layout(rgba16f, set=0, binding=0) uniform image2D scene;
layout(set=0, binding=1) uniform sampler2D depth_image;
layout(rgba16f, set=0, binding=2) uniform image2D previous_image;
layout(set=0, binding=3) uniform sampler2D radial_mask;
layout(set=0, binding=4) uniform sampler2D filter_image;
layout(rgba16f, set=0, binding=5) uniform image2D focus_image;
layout(push_constant, std430) uniform State {
    vec4 fog;
    vec4 focus;
    vec4 fog_params; // start slot, power, GS near depth, mode (-1 = off)
    vec4 filter_color;
    vec4 effects; // inner, outer, fixed filter coefficient, monochrome
    vec4 camera; // near, far, history mode, dispatch phase
    vec4 kernel; // transition slots, tap count, tap blend, unused
    vec4 spare;
} p;
vec3 to_srgb(vec3 v) {
    v = max(v, vec3(0));
    return mix(v * 12.92, 1.055 * pow(v, vec3(1.0/2.4)) - 0.055, greaterThan(v, vec3(0.0031308)));
}
vec3 to_linear(vec3 v) {
    v = max(v, vec3(0));
    return mix(v / 12.92, pow((v + 0.055) / 1.055, vec3(2.4)), greaterThan(v, vec3(0.04045)));
}
float depth_slot(float depth) {
    return clamp(floor(p.fog_params.z * depth / 256.0), 0.0, 255.0);
}
float slot_at(float distance) {
    float n = p.camera.x, f = p.camera.y;
    return p.fog_params.z / 256.0 * ((n*f/max(n,distance)-n)/(f-n));
}
void main() {
    ivec2 xy = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(scene);
    if (p.camera.w == 1.0) {
        ivec2 half_size = imageSize(focus_image);
        if (any(greaterThanEqual(xy, half_size))) return;
        const vec2 taps[6] = vec2[6](vec2(-1), vec2(1), vec2(1,-1), vec2(1), vec2(-1,1), vec2(-1));
        vec3 blurred = vec3(0);
        for (int i=0; i<int(p.kernel.y); ++i) {
            ivec2 offset = ivec2(round(taps[i] * vec2(size) / vec2(640,480)));
            ivec2 uv = clamp(xy*2+offset, ivec2(0), size-1);
            vec3 sample_color = to_srgb(imageLoad(scene, uv).rgb);
            blurred = i==0 ? sample_color : mix(blurred, sample_color, p.kernel.z);
        }
        imageStore(focus_image, xy, vec4(blurred,1));
        return;
    }
    if (any(greaterThanEqual(xy, size))) return;
    vec4 c = imageLoad(scene, xy);
    vec2 uv = (vec2(xy)+0.5)/vec2(size);
    float slot = depth_slot(texelFetch(depth_image, xy, 0).r);
    if (p.fog_params.w >= 0.0) {
        float far_slot = 255.0 - slot;
        float density = clamp((far_slot - p.fog_params.x) / max(1.0, 255.0 - p.fog_params.x), 0.0, 1.0);
        float weight = floor(pow(max(0.01,density), p.fog_params.y) * p.fog.a * 128.0) / 128.0;
        vec3 rgb = p.fog.rgb;
        if (p.fog_params.w == 1.0) { rgb = vec3(0); weight = (1.0-density)*p.fog.a; }
        if (p.fog_params.w == 2.0) { rgb = vec3(slot/255.0); weight = 1.0; }
        if (p.fog_params.w == 3.0) weight = slot == trunc(p.fog_params.x) ? p.fog.a : 0.0;
        if (weight > 0.0) c.rgb = to_linear(mix(to_srgb(c.rgb), rgb, weight));
    }
    if (p.camera.w == 2.0) { imageStore(scene, xy, c); return; }
    if (p.kernel.y > 0.0) {
        float near_slot = clamp(floor(slot_at(p.focus.x-p.focus.y*0.5)),0.0,255.0);
        float far_slot = clamp(floor(slot_at(p.focus.x+p.focus.y*0.5)),0.0,255.0);
        float edge = max(max(slot-near_slot, far_slot-slot), 0.0);
        float alpha = mix(p.focus.z, p.focus.w, clamp(edge/max(p.kernel.x,1.0),0.0,1.0));
        ivec2 limit = imageSize(focus_image)-1;
        ivec2 base = ivec2(floor(vec2(xy)*0.5-0.25));
        vec2 f = fract(vec2(xy)*0.5-0.25);
        vec3 a = mix(imageLoad(focus_image,clamp(base,ivec2(0),limit)).rgb,
            imageLoad(focus_image,clamp(base+ivec2(1,0),ivec2(0),limit)).rgb,f.x);
        vec3 b = mix(imageLoad(focus_image,clamp(base+ivec2(0,1),ivec2(0),limit)).rgb,
            imageLoad(focus_image,clamp(base+ivec2(1),ivec2(0),limit)).rgb,f.x);
        c.rgb = to_linear(mix(to_srgb(c.rgb), mix(a,b,f.y), alpha));
    }
    if (p.effects.w > 0.0) c.rgb = vec3(c.g);
    if (p.effects.z > 0.0) {
        vec3 filter_color = texture(filter_image, uv).rgb * p.filter_color.rgb;
        vec3 dst = to_srgb(c.rgb);
        c.rgb = to_linear(clamp(dst + (dst-filter_color)*p.effects.z, 0.0, 1.0));
    }
    if (p.camera.z > 0.0) {
        vec2 quadrant = 1.0 - abs(2.0 * uv - 1.0);
        float mask = texture(radial_mask, quadrant).a;
        float weight = clamp(mask * mix(p.effects.y, p.effects.x, min(quadrant.x,quadrant.y)), 0.0, 1.0);
        if (p.camera.z > 1.0) c.rgb = to_linear(mix(to_srgb(c.rgb), to_srgb(imageLoad(previous_image,xy).rgb), weight));
        imageStore(previous_image, xy, c);
    }
    imageStore(scene, xy, c);
}
)";

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
    set_effect_callback_type(EFFECT_CALLBACK_TYPE_POST_TRANSPARENT);
    set_access_resolved_color(true);
    set_access_resolved_depth(true);
    auto image = Image::create_empty(1, 1, false, Image::FORMAT_RGBA8);
    image->fill(Color(1,1,1,1));
    neutral_ = ImageTexture::create_from_image(image);
}

void RacePostPresentation::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_test_mode", "mode"), &RacePostPresentation::set_test_mode);
}

void RacePostPresentation::set_test_mode(int mode) {
    if (OS::get_singleton()->get_environment("HP2_MVP_TEST_POST") != "1") return;
    std::lock_guard<std::mutex> lock(mutex_);
    testMode_ = mode;
    ++epoch_;
    set_enabled(true);
}

RacePostPresentation::~RacePostPresentation() {
    if (auto* server = RenderingServer::get_singleton()) {
        if (auto* rd = server->get_rendering_device()) {
            if (shader_.is_valid()) rd->free_rid(shader_);
            if (sampler_.is_valid()) rd->free_rid(sampler_);
        }
    }
}

void RacePostPresentation::update(const RacePostState& s, bool reset, const RaceEffectOptions& options) {
    std::lock_guard<std::mutex> lock(mutex_);
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
    const bool radial = options.motionBlur && s.radialActive && mask_.is_valid();
    const bool fog = options.fog && s.fogEnabled && (s.fog[3] > 0.0f || s.fogMode != 0);
    const bool monochrome = options.monochrome && s.monochrome;
    const float filterAlpha = options.colorFilter && filter_.is_valid() ? s.filterAlpha / 128.0f : 0.0f;
    const int focusPasses = options.depthOfField && s.focusEnabled ? std::clamp(s.focusPasses, 0, 6) : 0;
    reset |= radial != radialActive_;
    radialActive_ = radial;
    if (reset) ++epoch_;
    parameters_ = {s.fog[0]/255, s.fog[1]/255, s.fog[2]/255, s.fog[3],
        s.focus[0], s.focus[1], s.focus[2]/128, s.focus[3]/128,
        s.fogRange[0], s.fogPower, s.fogRange[1], fog ? float(s.fogMode) : -1.0f,
        s.filterColor[0]/128.0f, s.filterColor[1]/128.0f, s.filterColor[2]/128.0f, 0,
        s.radialAmounts[0]/128.0f, s.radialAmounts[1]/128.0f, filterAlpha,
        monochrome ? 1.0f : 0.0f, 0, 0, radial ? 1.0f : 0.0f, 0,
        float(s.focusTransition), float(focusPasses),
        s.focusJitter/128.0f, 0};
    const bool enabled = fog || focusPasses > 0 || monochrome || radial || filterAlpha > 0.0f;
    if (enabled != enabled_) {
        set_enabled(enabled);
        enabled_ = enabled;
    }
}

void RacePostPresentation::_render_callback(int32_t type, RenderData* data) {
    if (type != EFFECT_CALLBACK_TYPE_POST_TRANSPARENT || !data) return;
    auto* server = RenderingServer::get_singleton();
    auto* rd = server->get_rendering_device();
    const Ref<RenderSceneBuffersRD> buffers = data->get_render_scene_buffers();
    if (!rd || buffers.is_null() || buffers->get_view_count() != 1) return;
    const auto size = buffers->get_internal_size();
    if (size.x < 1 || size.y < 1) return;
    if (!shader_.is_valid()) {
        Ref<RDShaderSource> source;
        source.instantiate();
        source->set_stage_source(RenderingDevice::SHADER_STAGE_COMPUTE, shaderCode);
        const auto spirv = rd->shader_compile_spirv_from_source(source);
        const auto error = spirv->get_stage_compile_error(RenderingDevice::SHADER_STAGE_COMPUTE);
        if (!error.is_empty()) { UtilityFunctions::push_error(error); return; }
        shader_ = rd->shader_create_from_spirv(spirv, "HP2 Race Post");
        pipeline_ = rd->compute_pipeline_create(shader_);
        Ref<RDSamplerState> state;
        state.instantiate();
        state->set_min_filter(RenderingDevice::SAMPLER_FILTER_LINEAR);
        state->set_mag_filter(RenderingDevice::SAMPLER_FILTER_LINEAR);
        sampler_ = rd->sampler_create(state);
        for (std::size_t i = 0; i < bindings_.size(); ++i) {
            auto& binding = bindings_[i];
            binding.instantiate();
            binding->set_binding(i);
            binding->set_uniform_type(i == 0 || i == 2 || i == 5 ? RenderingDevice::UNIFORM_TYPE_IMAGE :
                RenderingDevice::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE);
            uniforms_.push_back(binding);
        }
        pushConstants_.resize(sizeof(parameters_));
    }
    std::array<float,32> p;
    RID mask, filter;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        p = parameters_;
        mask = server->texture_get_rd_texture((mask_.is_valid() ? mask_ : neutral_)->get_rid());
        filter = server->texture_get_rd_texture((filter_.is_valid() ? filter_ : neutral_)->get_rid());
        if (renderedEpoch_ != epoch_) { historyValid_ = false; renderedEpoch_ = epoch_; }
        if (testMode_ != 0) {
            p[11] = -1;
            p[18] = p[19] = p[22] = p[25] = 0;
            if (testMode_ == 1) p[19] = 1;
            if (testMode_ == 2) { p[4] = 20; p[5] = 2; p[6] = 0; p[7] = 1; p[25] = 2; }
            if (testMode_ == 3) { p[16] = 0.35f; p[17] = 0.75f; p[22] = 1; }
            if (testMode_ == 4) { p[12] = 0.8f; p[13] = 0.3f; p[14] = 0.2f; p[18] = 0.7f; }
        }
    }
    const auto projection = data->get_render_scene_data()->get_cam_projection();
    p[20] = projection.get_z_near();
    p[21] = projection.get_z_far();
    const auto color = buffers->get_color_layer(0);
    const auto depth = buffers->get_depth_layer(0);
    const Vector2i halfSize((size.x + 1)/2, (size.y + 1)/2);
    const auto auxiliary = [&](const StringName& name, const Vector2i& extent) {
        if (buffers->has_texture("hp2_post", name)) return buffers->get_texture("hp2_post", name);
        return buffers->create_texture("hp2_post", name, RenderingDevice::DATA_FORMAT_R16G16B16A16_SFLOAT,
            RenderingDevice::TEXTURE_USAGE_STORAGE_BIT, RenderingDevice::TEXTURE_SAMPLES_1, extent, 1, 1, true, false);
    };
    const auto scratch = auxiliary("unused", Vector2i(1,1));
    const auto history = p[22] > 0 ? auxiliary("history", size) : scratch;
    const auto focus = p[25] > 0 ? auxiliary("focus", halfSize) : auxiliary("unused_focus", Vector2i(1,1));
    if (history != previousColor_) { historyValid_ = false; previousColor_ = history; }
    if (p[22] > 0) {
        p[22] = historyValid_ ? 2 : 1;
        historyValid_ = true;
    }
    const std::array<RID,6> resources{color, depth, history, mask, filter, focus};
    for (std::size_t i = 0; i < bindings_.size(); ++i) {
        auto& binding = bindings_[i];
        binding->clear_ids();
        if (i == 1 || i == 3 || i == 4) binding->add_id(sampler_);
        binding->add_id(resources[i]);
    }
    const auto set = UniformSetCacheRD::get_cache(shader_, 0, uniforms_);
    const auto dispatch = [&](Vector2i extent, float phase) {
        p[23] = phase;
        std::memcpy(pushConstants_.ptrw(), p.data(), sizeof(p));
        const auto list = rd->compute_list_begin();
        rd->compute_list_bind_compute_pipeline(list, pipeline_);
        rd->compute_list_bind_uniform_set(list, set, 0);
        rd->compute_list_set_push_constant(list, pushConstants_, pushConstants_.size());
        rd->compute_list_dispatch(list, (extent.x + 7)/8, (extent.y + 7)/8, 1);
        rd->compute_list_end();
    };
    if (p[25] > 0) {
        if (p[11] >= 0) dispatch(size, 2);
        p[11] = -1;
        dispatch(halfSize, 1);
    }
    dispatch(size, 0);
}
}
