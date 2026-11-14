shader_type canvas_item;
render_mode unshaded;
uniform sampler2D frame_texture : repeat_disable, filter_linear;
uniform sampler2D mask_texture : repeat_disable, filter_linear;
varying vec4 tint;
void vertex() {
    tint = COLOR;
}
void fragment() {
    vec2 scene_uv = vec2(1.0 - UV.x, UV.y);
    vec2 overlay_uv = vec2(UV.x + 1.0 / 256.0, (UV.y * 64.0 + 14.0) / 90.0);
    vec4 frame = texture(frame_texture, overlay_uv);
    vec4 mask = texture(mask_texture, overlay_uv);
    vec3 rear = texture(TEXTURE, scene_uv).rgb;
    COLOR = vec4(mix(rear, frame.rgb, frame.a) + mask.rgb * mask.a, mask.a) * tint;
}
