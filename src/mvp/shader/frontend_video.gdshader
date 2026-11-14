shader_type canvas_item;
render_mode unshaded;
uniform sampler2D chroma_u : filter_linear, repeat_disable;
uniform sampler2D chroma_v : filter_linear, repeat_disable;
void fragment() {
    vec2 uv = vec2(UV.x, 1.0 - UV.y);
    float y = texture(TEXTURE, uv).r;
    float u = texture(chroma_u, uv).r - 128.0 / 255.0;
    float v = texture(chroma_v, uv).r - 128.0 / 255.0;
    COLOR = vec4(y + 1.402 * v, y - 0.344 * u - 0.714 * v, y + 1.772 * u, 1.0);
}
