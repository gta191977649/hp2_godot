shader_type canvas_item;
render_mode unshaded, blend_disabled;

#include "race_post_shared.gdshaderinc"

void fragment() {
    ivec2 size = textureSize(scene_color, 0);
    ivec2 xy = clamp(ivec2(UV * vec2(size)), ivec2(0), size - 1);
    COLOR = vec4(fogged(xy), 1.0);
}
