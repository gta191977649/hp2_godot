shader_type canvas_item;
render_mode unshaded, blend_disabled;

uniform sampler2D fallback_scene : filter_nearest, repeat_disable;
uniform sampler2D fallback_fxaa_scene : filter_linear, repeat_disable;
uniform sampler2D post_color : filter_nearest, repeat_disable;
uniform sampler2D post_fxaa_color : filter_linear, repeat_disable;
uniform bool use_post;
uniform bool seed_history;
uniform bool fxaa;
uniform bool fallback_linear;

float luma(vec3 color) { return dot(color, vec3(0.299, 0.587, 0.114)); }

vec3 antialias(sampler2D image, vec2 uv, vec3 center) {
    vec2 pixel = 1.0 / vec2(textureSize(image, 0));
    float nw = luma(texture(image, uv + pixel * vec2(-1.0,-1.0)).rgb);
    float ne = luma(texture(image, uv + pixel * vec2(1.0,-1.0)).rgb);
    float sw = luma(texture(image, uv + pixel * vec2(-1.0,1.0)).rgb);
    float se = luma(texture(image, uv + pixel * vec2(1.0,1.0)).rgb);
    float m = luma(center);
    float low = min(m, min(min(nw,ne), min(sw,se)));
    float high = max(m, max(max(nw,ne), max(sw,se)));
    if (high-low < max(0.0312, high*0.125)) return center;
    vec2 direction = vec2(sw+se-nw-ne, nw+sw-ne-se);
    float reduce = max((nw+ne+sw+se)*0.03125, 0.0078125);
    direction = clamp(direction / (min(abs(direction.x),abs(direction.y))+reduce), vec2(-8.0), vec2(8.0))*pixel;
    vec3 inner = (texture(image,uv-direction/6.0).rgb + texture(image,uv+direction/6.0).rgb)*0.5;
    vec3 outer = inner*0.5 + (texture(image,uv-direction*0.5).rgb + texture(image,uv+direction*0.5).rgb)*0.25;
    float edge = luma(outer);
    return edge < low || edge > high ? inner : outer;
}

void fragment() {
    bool show_post = use_post && !seed_history;
    COLOR = show_post ? texture(post_color, UV) : texture(fallback_scene, UV);
    if (!show_post && fallback_linear) {
        vec3 color = max(COLOR.rgb, vec3(0.0));
        COLOR.rgb = mix(color * 12.92, 1.055 * pow(color, vec3(1.0/2.4)) - 0.055,
            greaterThan(color, vec3(0.0031308)));
    }
    if (fxaa && !seed_history) {
        COLOR.rgb = show_post ? antialias(post_fxaa_color, UV, COLOR.rgb) :
            antialias(fallback_fxaa_scene, UV, COLOR.rgb);
    }
}
