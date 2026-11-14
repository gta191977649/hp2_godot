shader_type canvas_item;
render_mode unshaded, blend_disabled;

void fragment() {
    vec3 color = clamp(texture(TEXTURE, UV).rgb, vec3(0.0), vec3(1.0));
    COLOR = vec4(floor(color * 255.0 * 96.0 / 128.0) / 255.0, 64.0 / 255.0);
}
