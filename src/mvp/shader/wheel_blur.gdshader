shader_type spatial;
render_mode unshaded, cull_disabled, shadows_disabled, depth_draw_never, blend_mix;

uniform sampler2D wheel_blur_texture : filter_linear_mipmap, repeat_enable;
uniform ivec2 repeat_axes;
uniform bool alpha_test;
instance uniform float hp2_wheel_blur_alpha;

void fragment() {
    vec2 margin = 0.5 / vec2(textureSize(wheel_blur_texture, 0));
    vec2 uv = mix(clamp(UV, margin, 1.0 - margin), UV, notEqual(repeat_axes, ivec2(0)));
    vec4 texel = texture(wheel_blur_texture, uv);
    float alpha = floor(texel.a * 255.0 * floor(hp2_wheel_blur_alpha) / 128.0);
    if (alpha <= (alpha_test ? 48.0 : 0.0)) discard;
    vec3 color = clamp(texel.rgb * COLOR.rgb * (31.0 / 16.0), 0.0, 1.0);
    ALBEDO = OUTPUT_IS_SRGB ? color : mix(color / 12.92,
        pow((color + 0.055) / 1.055, vec3(2.4)), step(vec3(0.04045), color));
    ALPHA = min(alpha / 128.0, 1.0);
}
