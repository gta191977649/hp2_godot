uniform sampler2D scene_color : filter_nearest, repeat_disable;
uniform sampler2D scene_depth : filter_nearest, repeat_disable;
uniform vec4 fog;
uniform vec4 focus;
uniform vec4 fog_params;
uniform vec4 filter_color;
uniform vec4 effects;
uniform vec4 camera;
uniform vec4 kernel;
uniform vec4 history;
uniform sampler2D fog_palette : filter_nearest, repeat_disable;
uniform bool use_fog_palette;

vec3 to_srgb(vec3 v) {
    v = max(v, vec3(0));
    return mix(v * 12.92, 1.055 * pow(v, vec3(1.0/2.4)) - 0.055, greaterThan(v, vec3(0.0031308)));
}
vec3 to_linear(vec3 v) {
    v = max(v, vec3(0));
    return mix(v / 12.92, pow((v + 0.055) / 1.055, vec3(2.4)), greaterThan(v, vec3(0.04045)));
}
float depth_slot(float depth) {
    return clamp(floor(fog_params.z * depth / 256.0), 0.0, 255.0);
}
float slot_at(float distance) {
    float n = camera.x, f = camera.y;
    return fog_params.z / 256.0 * ((n*f/max(n,distance)-n)/(f-n));
}
float scene_depth_at(ivec2 xy) {
    ivec2 color_size = textureSize(scene_color, 0);
    ivec2 depth_size = textureSize(scene_depth, 0);
    if (color_size != depth_size) {
        xy = ivec2((vec2(xy) + 0.5) * vec2(depth_size) / vec2(color_size));
    }
    return texelFetch(scene_depth, xy, 0).r;
}
vec3 fogged(ivec2 xy, bool encoded) {
    vec3 color = texelFetch(scene_color, xy, 0).rgb;
    if (fog_params.w >= 0.0) {
        float slot = depth_slot(scene_depth_at(xy));
        if (use_fog_palette) {
            vec4 entry = texelFetch(fog_palette, ivec2(int(slot), 0), 0);
            if (entry.a > 0.0) {
                vec3 result = mix(to_srgb(color), entry.rgb, entry.a);
                return encoded ? max(result, vec3(0.0)) : to_linear(result);
            }
            return encoded ? to_srgb(color) : color;
        }
        float far_slot = 255.0 - slot;
        float density = clamp((far_slot - fog_params.x) / max(1.0, 255.0 - fog_params.x), 0.0, 1.0);
        float weight = floor(pow(max(0.01,density), fog_params.y) * fog.a * 128.0) / 128.0;
        vec3 rgb = fog.rgb;
        if (fog_params.w == 1.0) { rgb = vec3(0); weight = (1.0-density)*fog.a; }
        if (fog_params.w == 2.0) { rgb = vec3(slot/255.0); weight = 1.0; }
        if (fog_params.w == 3.0) weight = slot == trunc(fog_params.x) ? fog.a : 0.0;
        if (weight > 0.0) {
            vec3 result = mix(to_srgb(color), rgb, weight);
            return encoded ? result : to_linear(result);
        }
    }
    return encoded ? to_srgb(color) : color;
}
