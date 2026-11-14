uniform sampler2D atlas : source_color, filter_linear, repeat_disable;
uniform sampler2D scene_depth : hint_depth_texture, filter_nearest, repeat_disable;
uniform vec4 color_curve[4];
uniform vec4 size_curve[4];
uniform vec2 atlas_size;
uniform float lifetime;
uniform float pixel_cap;
uniform float interest_distance;
uniform float alpha_cutoff;
uniform int texture_function;
uniform sampler2D group_epochs : filter_nearest, repeat_disable;
varying vec4 tint;
varying flat float visible_in_view;
varying flat float particle_depth;

void vertex() {
    float age = 1.0 - float(int(INSTANCE_CUSTOM.x) & 65535) / (1024.0 * lifetime);
    vec4 shape = ((size_curve[0] * age + size_curve[1]) * age + size_curve[2]) * age + size_curve[3];
    tint = clamp(((color_curve[0] * age + color_curve[1]) * age + color_curve[2]) * age + color_curve[3],
                 vec4(0.0), vec4(255.0)) / 128.0;
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
    particle_depth = depth;
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
    vec4 texel = texture(atlas, UV);
    vec4 result = texel * tint;
    if (texture_function == 1) result = texel;
    if (texture_function >= 2) {
        result.rgb += tint.a;
        result.a = texel.a + (texture_function == 2 ? tint.a : 0.0);
    }
    ALBEDO = result.rgb;
    ALPHA = clamp(result.a, 0.0, 1.0);
    if (ALPHA <= alpha_cutoff) discard;
    float depth = textureLod(scene_depth, SCREEN_UV, 0.0).r;
    if (depth > 0.0) {
        vec4 scene_view = INV_PROJECTION_MATRIX * vec4(SCREEN_UV * 2.0 - 1.0, depth, 1.0);
        float gap = -scene_view.z / scene_view.w - particle_depth;
        ALPHA *= smoothstep(0.0, 0.3, gap);
    }
}
