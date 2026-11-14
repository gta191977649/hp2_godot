shader_type spatial;
render_mode unshaded, blend_mix, cull_disabled, depth_draw_never, shadows_disabled;

uniform sampler2D shadow_texture : source_color, filter_linear, repeat_disable;

void fragment() {
    vec2 size = vec2(textureSize(shadow_texture, 0));
    vec2 uv = clamp(UV, vec2(0.5) / size, 1.0 - vec2(0.5) / size);
    vec2 texels = UV * size;
    float footprint = max(length(dFdx(texels)), length(dFdy(texels)));
    vec4 texel = footprint > 1.0
        ? texelFetch(shadow_texture, ivec2(uv * size), 0) : texture(shadow_texture, uv);
    ALBEDO = texel.rgb;
    float alpha_byte = floor(texel.a * COLOR.a * (255.0 * 255.0 / 128.0) + 0.001);
    ALPHA = min(alpha_byte / 128.0, 1.0);
}
