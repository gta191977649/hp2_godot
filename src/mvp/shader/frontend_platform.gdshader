shader_type spatial;
render_mode unshaded, cull_disabled, depth_draw_never, blend_premul_alpha;

uniform sampler2D base_image : filter_linear_mipmap, repeat_enable;
uniform sampler2D opaque_scene : hint_screen_texture, filter_nearest, repeat_disable;
uniform ivec4 base_sampling;
uniform float base_lod_scale;
instance uniform vec3 hp2_paint_focal;

varying vec4 color_weighted;
varying float clip_weight;
varying flat float mip_bias;

#include "gs_byte_texture.gdshaderinc"

void vertex() {
    clip_weight = (PROJECTION_MATRIX * MODELVIEW_MATRIX * vec4(VERTEX, 1.0)).w;
    color_weighted = round(COLOR * 255.0) * clip_weight;
    mip_bias = CUSTOM2.x;
}

void fragment() {
    float lod = log2(max(clip_weight / hp2_paint_focal.x, 1e-12)) * base_lod_scale + mip_bias;
    vec4 rgba = floor(color_weighted / clip_weight + 0.01);
    vec4 base = paint_modulate(rgba, paint_sample(base_image, UV, base_sampling, lod));
    vec4 backdrop = texture(opaque_scene, SCREEN_UV);
    float opacity = min(base.a / 128.0, 1.0);
    vec3 destination = backdrop.a >= 0.999 ? gs_byte_rgb(backdrop.rgb, OUTPUT_IS_SRGB) : vec3(0.0);
    vec3 encoded = clamp(destination + floor((base.rgb - destination) * base.a / 128.0), vec3(0.0), vec3(255.0)) / 255.0;
    ALBEDO = OUTPUT_IS_SRGB ? encoded : gs_encoded_to_linear(encoded);
    ALPHA = backdrop.a >= 0.999 ? 1.0 : opacity;
}
