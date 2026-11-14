shader_type canvas_item;
render_mode unshaded, blend_disabled;

void fragment() {
    float right_edge = round(1.0 / SCREEN_PIXEL_SIZE.x) - 1.0;
    float red = floor(FRAGCOORD.x) == 0.0 || floor(FRAGCOORD.x) == right_edge ? 0.0 : 1.0;
    COLOR = vec4(red, 0.0, 0.0, 1.0);
}
