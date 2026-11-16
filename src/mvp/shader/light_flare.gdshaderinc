#ifdef GS_FLARE_ADD
uniform sampler2D flare : filter_nearest, repeat_disable;
uniform sampler2D flare_scene : hint_screen_texture, filter_nearest, repeat_disable;
uniform ivec4 flare_sampling;
varying flat float flare_lod;
#include "gs_byte_texture.gdshaderinc"
#else
uniform sampler2D flare : source_color, filter_linear, repeat_disable;
#endif
uniform bool track_batch;
instance uniform vec4 normal_and_sides;
instance uniform vec4 response;
instance uniform vec4 distance_scale;
instance uniform vec4 minimum_color;
instance uniform vec4 maximum_color;
instance uniform float focal_scale = 240.0;
varying vec4 tint;

void vertex() {
    vec3 center = track_batch ? (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz : MODEL_MATRIX[3].xyz;
    vec4 sides = track_batch ? CUSTOM0 : normal_and_sides;
    vec4 curve = track_batch ? CUSTOM1 : response;
    vec4 scale = track_batch ? CUSTOM2 : distance_scale;
    vec4 low = track_batch ? round(COLOR * 255.0) / 128.0 : minimum_color;
    vec4 high = track_batch ? CUSTOM3 / 128.0 : maximum_color;
    vec3 delta = INV_VIEW_MATRIX[3].xyz - center;
    vec3 to_camera = delta * inversesqrt(max(dot(delta, delta), 1e-12));
    float facing = dot(sides.xyz, to_camera);
    if (sides.w > 0.5) facing = abs(facing);
    // RenderSingleFlare uses a float-bit power curve on the EE.
    float angle = max(facing, 0.01);
    float intercept = 1065353216.0 - curve.z * 1065353216.0;
    float blend = intBitsToFloat(int(float(floatBitsToInt(angle)) * curve.z + intercept)) * curve.w;
    vec4 view_center = VIEW_MATRIX * vec4(center, 1.0);
    float focal = focal_scale * abs(PROJECTION_MATRIX[1][1]);
    float size = mix(curve.x, curve.y, blend) * clamp(-view_center.z / focal, scale.x, scale.y);
    vec4 biased = VIEW_MATRIX * vec4(center + to_camera * scale.z, 1.0);
#ifdef GS_FLARE_ADD
    flare_lod = log2(max(-biased.z / focal, 1e-12));
#endif
    vec2 corner = track_batch ? vec2(UV.x - 0.5, 0.5 - UV.y) : VERTEX.xy;
    POSITION = PROJECTION_MATRIX * (biased + vec4(corner * 2.0 * size, 0.0, 0.0));
    if (track_batch) {
        // Source clips the marker before applying bias or expanding the quad.
        vec4 projected = PROJECTION_MATRIX * view_center;
        if (projected.w <= 0.0 || any(greaterThan(abs(projected.xyz), vec3(projected.w))))
            POSITION = vec4(2.0, 2.0, 2.0, 1.0);
    }
    tint = vec4(ivec4((low + (high - low) * blend) * 128.0) & ivec4(255));
}

void fragment() {
#ifdef GS_FLARE_ADD
    // The authored flare textures all use TFX MODULATE and ALPHA 0x48:
    // saturate the modulated bytes, then add floor(Cs * As / 128) to Cd.
    vec4 source = paint_modulate(tint, paint_sample(flare, UV, flare_sampling, flare_lod));
    vec3 addition = floor(source.rgb * source.a / 128.0);
    if (all(equal(addition, vec3(0.0)))) discard;
    vec3 background = texture(flare_scene, SCREEN_UV).rgb;
    vec3 gs_background = gs_byte_rgb(background, OUTPUT_IS_SRGB);
    vec3 result = clamp(gs_background + addition, vec3(0.0), vec3(255.0)) / 255.0;
    vec3 linear_result = OUTPUT_IS_SRGB ? result : gs_encoded_to_linear(result);
    // Keep additive ordering and put only the change to the opaque scene
    // in the blend unit. The screen snapshot cannot see prior alpha draws.
    ALBEDO = max(linear_result - background, vec3(0.0));
    ALPHA = 1.0;
#else
    vec4 texel = texture(flare, UV);
    ALBEDO = texel.rgb * tint.rgb / 128.0;
    ALPHA = clamp(texel.a * tint.a / 128.0, 0.0, 1.0);
#endif
}
