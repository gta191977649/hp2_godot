uniform sampler2D atlas : filter_linear, repeat_disable;
uniform vec4 color_curve[4];
uniform vec4 size_curve[4];
uniform vec2 atlas_size;
uniform float lifetime;
uniform float pixel_cap;
uniform float interest_distance;
uniform bool alpha_test;
uniform bool nearest_minification;
uniform int texture_function;
uniform sampler2D group_epochs : filter_nearest, repeat_disable;
varying vec4 tint;
varying flat float visible_in_view;

void vertex() {
    float age = 1.0 - float(int(INSTANCE_CUSTOM.x) & 65535) / (1024.0 * lifetime);
    vec4 shape = ((size_curve[0] * age + size_curve[1]) * age + size_curve[2]) * age + size_curve[3];
    tint = floor(clamp(((color_curve[0] * age + color_curve[1]) * age + color_curve[2]) * age + color_curve[3],
                 vec4(0.0), vec4(255.0)));
    int angle = int(INSTANCE_CUSTOM.y) & 65535;
    float rotation = float(angle + (((angle & 1) == 0) ? int(shape.y) : -int(shape.y))) * TAU / 65536.0;
    vec2 offset = mat2(vec2(cos(rotation), sin(rotation)), vec2(-sin(rotation), cos(rotation))) * VERTEX.xy;
    vec4 center = VIEW_MATRIX * MODEL_MATRIX[3];
    float depth = -center.z;
    // Forward+ may flip the projection Y axis for a render target. Its
    // magnitude sets the focal length; using the sign collapses large FX.
    float focal = 240.0 * abs(PROJECTION_MATRIX[1][1]);
    float size = clamp(shape.x, 0.0, pixel_cap * max(depth, 0.0) / focal);
    int identity = int(INSTANCE_CUSTOM.w);
    vec4 group = texelFetch(group_epochs, ivec2(identity & 1023, 0), 0);
    int mask = int(group.g);
    int view = int(CAMERA_VISIBLE_LAYERS) & 6;
    visible_in_view = float(int(group.r) == (identity >> 10) && (mask & view) != 0 &&
        depth > 0.0 && depth < interest_distance && size > 0.0);
    // Keep the billboard in the standard spatial vertex path. This gives
    // Forward+ the same view-space position for depth, fog and rasterization.
    MODELVIEW_MATRIX = mat4(vec4(1.0, 0.0, 0.0, 0.0), vec4(0.0, 1.0, 0.0, 0.0),
        vec4(0.0, 0.0, 1.0, 0.0), center);
    VERTEX = vec3(offset * size * vec2(1.0, 0.875), 0.0);
    float frame = float(int(INSTANCE_CUSTOM.z) & 255);
    UV = (UV + vec2(mod(frame, atlas_size.x), floor(frame / atlas_size.x))) / atlas_size;
}

void fragment() {
    if (visible_in_view < 0.5) discard;
    vec2 size = vec2(textureSize(atlas, 0));
    vec2 texel_uv = clamp(UV, vec2(0.5) / size, 1.0 - vec2(0.5) / size);
    vec2 texel_position = UV * size;
    float footprint = max(length(dFdx(texel_position)), length(dFdy(texel_position)));
    vec4 texel = (nearest_minification && footprint > 1.0
        ? texelFetch(atlas, ivec2(texel_uv * size), 0) : texture(atlas, texel_uv)) * 255.0;
    vec4 result = texel * tint / 128.0;
    if (texture_function == 1) result = texel;
    if (texture_function >= 2) {
        result.rgb += tint.a;
        result.a = texel.a + (texture_function == 2 ? tint.a : 0.0);
    }
    result = min(floor(result + 0.001), vec4(255.0));
    if (result.a <= (alpha_test ? 48.0 : 0.0)) discard;
    vec3 rgb = result.rgb / 255.0;
    ALBEDO = mix(rgb / 12.92, pow((rgb + 0.055) / 1.055, vec3(2.4)),
        greaterThan(rgb, vec3(0.04045)));
#ifndef PARTICLE_CUTOUT
    ALPHA = min(result.a / 128.0, 1.0);
#endif
}
