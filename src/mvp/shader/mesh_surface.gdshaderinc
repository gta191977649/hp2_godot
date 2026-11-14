uniform bool use_morph;
uniform bool use_rigid;
uniform bool use_scroll;
uniform bool use_shaft;
uniform bool face_camera;

global uniform vec4 hp2_animation_clock; // interpolated ticks, texture seconds, real ticks, committed ticks
uniform ivec2 repeat_axes = ivec2(1);
uniform sampler2D albedo_texture : source_color, filter_linear_mipmap, repeat_enable;
uniform sampler2D albedo_magnified : source_color, filter_linear, repeat_enable;
uniform sampler2D albedo_minified : source_color, filter_nearest, repeat_enable;

uniform sampler2DArray frame_textures : source_color, filter_linear_mipmap, repeat_enable;
uniform int texture_count;
uniform float texture_rate;
uniform bool real_clock;
uniform vec2 scroll_rate;
uniform float scroll_interval;
uniform bool stepped_scroll;
uniform sampler2D vertex_frames : filter_nearest, repeat_disable;
uniform int frame_stride;
uniform int morph_count;
uniform float morph_rate;
vec4 sample_vertex(int frame, int vertex) {
    int i = frame * frame_stride + vertex;
    return texelFetch(vertex_frames, ivec2(i % 1024, i / 1024), 0);
}
uniform sampler2D rigid_frames : filter_nearest, repeat_disable;
uniform int rigid_count;
uniform float rigid_duration;
vec4 sample_rigid(int channel, int frame) {
    return texelFetch(rigid_frames, ivec2(channel, frame), 0);
}
uniform int animation_start;
instance uniform int hp2_animation_start = -1;

#ifdef TRACK_SKY
uniform bool sky_blend_alpha;
uniform bool sky_cutout;
#endif

#ifdef GS_TRACK
#include "track_material.gdshaderinc"
#endif

void vertex() {
    int start = hp2_animation_start >= 0 ? hp2_animation_start : animation_start;
    if (use_morph) {
        float phase = (hp2_animation_clock.x - float(start & 16383)) * morph_rate / 60.0;
        int frame = int(floor(phase));
        // Donor selects an unsigned remainder when the random start is ahead.
        int current = int(uint(frame % morph_count) % uint(morph_count));
        int next = int(uint((frame + 1) % morph_count) % uint(morph_count));
        int vertex = int(CUSTOM0.x);
        VERTEX = mix(sample_vertex(current, vertex).xyz, sample_vertex(next, vertex).xyz, fract(phase));
    }
    if (use_rigid) {
        if (start != 0) {
            float tick = float((int(hp2_animation_clock.x) - start) & 16383) + fract(hp2_animation_clock.x);
            float phase = tick / rigid_duration;
            bool loop = (start & 16384) != 0;
            if (loop || phase < float(rigid_count)) {
                int current = int(phase) % rigid_count;
                int next = loop ? (current + 1) % rigid_count : min(current + 1, rigid_count - 1);
                float weight = fract(phase);
                vec4 a = sample_rigid(0, current);
                vec4 b = sample_rigid(0, next);
                vec4 q = normalize(mix(a, dot(a, b) < 0.0 ? -b : b, weight));
                vec3 point = VERTEX * mix(sample_rigid(2, current).xyz, sample_rigid(2, next).xyz, weight);
                VERTEX = point + 2.0 * cross(q.xyz, cross(q.xyz, point) + q.w * point)
                    + mix(sample_rigid(1, current).xyz, sample_rigid(1, next).xyz, weight);
            }
        }
    }
    if (use_scroll) {
        float time = hp2_animation_clock.y;
        if (stepped_scroll) time = trunc(time / scroll_interval);
        vec2 offset = time * scroll_rate;
        UV += offset - trunc(offset);
    }
    if (face_camera) {
        vec3 origin = MODEL_MATRIX[3].xyz;
        vec2 delta = origin.xz - INV_VIEW_MATRIX[3].xz;
        vec2 direction;
        if (dot(delta, delta) > 1.0e-4) direction = normalize(delta);
        else if (abs(delta.y) < abs(delta.x)) direction = vec2(sign(delta.x), 0.0);
        else direction = vec2(0.0, delta.y > 0.0 ? 1.0 : -1.0);
        mat4 pose = mat4(vec4(direction.x, 0.0, direction.y, 0.0), vec4(0.0, 1.0, 0.0, 0.0),
            vec4(-direction.y, 0.0, direction.x, 0.0), vec4(origin, 1.0));
        MODELVIEW_MATRIX = VIEW_MATRIX * pose;
    }
#ifdef TRACK_SKY
    // StuffSky uses its own projection range. Preserve the view and UVs
    // while placing the sky behind world geometry, even in the 192 m mirror.
    POSITION = PROJECTION_MATRIX * MODELVIEW_MATRIX * vec4(VERTEX, 1.0);
    POSITION.z = CLIP_SPACE_FAR * POSITION.w;
#endif
#ifdef GS_TRACK
    // GS Gouraud RGBA is affine in screen space; UV still uses ST/Q.
    gs_clip_weight = (PROJECTION_MATRIX * MODELVIEW_MATRIX * vec4(VERTEX, 1.0)).w;
    gs_color_weighted = gs_vertex_color(VERTEX, COLOR, CUSTOM1.xyz, MODELVIEW_MATRIX,
        MODEL_MATRIX, INV_VIEW_MATRIX) * gs_clip_weight;
#endif
}

void fragment() {
#ifdef GS_TRACK
    vec4 result = gs_fragment(UV, OUTPUT_IS_SRGB);
    ALBEDO = result.rgb;
#if defined(GS_MIX) || defined(GS_ADD) || defined(GS_SUB)
    ALPHA = result.a;
#endif
#ifdef GS_BIASED
    DEPTH = gs_depth(-VERTEX.z, PROJECTION_MATRIX, CLIP_SPACE_FAR);
#endif
#else
    vec2 margin = 0.5 / vec2(textureSize(albedo_texture, 0));
    vec2 uv = mix(clamp(UV, margin, 1.0-margin), UV, notEqual(repeat_axes, ivec2(0)));
    vec4 color;
    if (texture_count > 0) {
        float clock = real_clock ? hp2_animation_clock.z : hp2_animation_clock.w;
        float frame = mod(trunc(texture_rate * (clock / 60.0)), float(texture_count));
        color = texture(frame_textures, vec3(uv, frame));
    } else if (use_shaft) {
        vec2 texels = UV * vec2(textureSize(albedo_texture, 0));
        float footprint = max(length(dFdx(texels)), length(dFdy(texels)));
        color = footprint > 1.0 ? texture(albedo_minified, uv) : texture(albedo_magnified, uv);
    } else {
        color = texture(albedo_texture, uv);
    }
    vec3 tint = COLOR.rgb;
    if (!OUTPUT_IS_SRGB) {
        tint = mix(tint / 12.92, pow((tint + 0.055) / 1.055, vec3(2.4)), step(vec3(0.04045), tint));
    }
    ALBEDO = color.rgb * tint;
#ifdef TRACK_SKY
    if (sky_cutout && color.a < 97.0 / 255.0) discard;
    ALPHA = sky_blend_alpha ? color.a : 1.0;
#endif
#ifdef CUTOUT
    ALPHA = color.a;
    ALPHA_SCISSOR_THRESHOLD = 97.0 / 255.0;
#endif
#ifdef TRANSPARENT
    ALPHA = color.a;
#endif
#endif
}
