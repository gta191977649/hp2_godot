shader_type canvas_item;
render_mode unshaded, blend_disabled;
uniform sampler2D scene_color : hint_screen_texture, repeat_disable, filter_nearest;
void fragment() {
    float green = texture(scene_color, SCREEN_UV).g;
    COLOR = vec4(vec3(green), 1.0);
}
