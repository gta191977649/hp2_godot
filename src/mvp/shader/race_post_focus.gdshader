shader_type canvas_item;
render_mode unshaded, blend_disabled;

#include "race_post_shared.gdshaderinc"

void fragment() {
    ivec2 xy = ivec2(FRAGCOORD.xy);
    ivec2 size = textureSize(scene_color, 0);
    const vec2 taps[6] = vec2[6](vec2(-1), vec2(1), vec2(1,-1), vec2(1), vec2(-1,1), vec2(-1));
    vec3 blurred = vec3(0);
    for (int i=0; i<int(kernel.y); ++i) {
        ivec2 offset = ivec2(round(taps[i] * vec2(size) / vec2(640,480)));
        ivec2 uv = clamp(xy*2+offset, ivec2(0), size-1);
        vec3 sample_color = fogged(uv);
        blurred = i==0 ? sample_color : mix(blurred, sample_color, kernel.z);
    }
    COLOR = vec4(blurred, 1.0);
}
