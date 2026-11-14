shader_type canvas_item;
render_mode unshaded, blend_disabled;

uniform sampler2D fallback_scene : filter_nearest, repeat_disable;
uniform sampler2D post_color : filter_nearest, repeat_disable;
uniform bool use_post;
uniform bool seed_history;

void fragment() {
    COLOR = use_post && !seed_history ? texture(post_color, UV) : texture(fallback_scene, UV);
}
