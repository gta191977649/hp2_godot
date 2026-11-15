uniform sampler2D atlas : filter_linear, repeat_disable;
uniform vec4 color_curve[4];
uniform vec4 size_curve[4];
uniform vec2 atlas_size;
uniform float lifetime;
uniform float pixel_cap;
uniform float preview_focal;
uniform float interest_distance;
uniform bool alpha_test;
uniform bool nearest_minification;
uniform int texture_function;
uniform sampler2D group_epochs : filter_nearest, repeat_disable;
uniform bool particle_baseline;
varying vec4 tint;
varying flat float visible_in_view;

void vertex() {
    int identity = int(INSTANCE_CUSTOM.w);
    int remaining = int(INSTANCE_CUSTOM.x) & 65535;
    vec4 group = texelFetch(group_epochs, ivec2(identity & 1023, 0), 0);
    int view = int(CAMERA_VISIBLE_LAYERS) & 6;
    vec4 center = VIEW_MATRIX * MODEL_MATRIX[3];
    float depth = -center.z;
    bool in_view = int(group.r) == (identity >> 10) && (int(group.g) & view) != 0 &&
        depth > 0.0 && depth < interest_distance;
    if (preview_focal > 0.0) {
        // RenderWorldParticles rejects an off-screen centre before expanding the quad.
        vec4 clip = PROJECTION_MATRIX * center;
        in_view = in_view && all(lessThanEqual(abs(clip.xy), vec2(clip.w))) &&
            clip.z >= CLIP_SPACE_FAR * clip.w && clip.z <= clip.w;
    }
    // Collapse rejected quads before rasterization. A fragment discard
    // still pays for every covered sample, especially with large smoke FX.
    if (!particle_baseline && (!in_view || remaining == 0)) {
        visible_in_view = 0.0;
        VERTEX = vec3(0.0);
    } else {
        float age = 1.0 - float(remaining) / (1024.0 * lifetime);
        vec4 shape = ((size_curve[0] * age + size_curve[1]) * age + size_curve[2]) * age + size_curve[3];
        tint = floor(clamp(((color_curve[0] * age + color_curve[1]) * age + color_curve[2]) * age + color_curve[3],
                     vec4(0.0), vec4(255.0)));
        int angle = int(INSTANCE_CUSTOM.y) & 65535;
        float rotation = float(angle + (((angle & 1) == 0) ? int(shape.y) : -int(shape.y))) * TAU / 65536.0;
        vec2 offset = mat2(vec2(cos(rotation), sin(rotation)), vec2(-sin(rotation), cos(rotation))) * VERTEX.xy;
        // Forward+ may flip the projection Y axis for a render target. Its
        // magnitude sets the focal length; using the sign collapses large FX.
        float focal = preview_focal > 0.0 ? preview_focal : 240.0 * abs(PROJECTION_MATRIX[1][1]);
        float size = clamp(shape.x, 0.0, pixel_cap * max(depth, 0.0) / focal);
        // GS modulate cannot produce a surviving alpha even with a fully
        // opaque texel if the quantized vertex alpha is below this limit.
        bool alpha_can_pass = texture_function != 0 ||
            floor(255.0 * tint.a / 128.0 + 0.001) > (alpha_test ? 48.0 : 0.0);
        bool visible_size = preview_focal > 0.0 ? size * focal > depth : size > 0.0;
        visible_in_view = float(in_view && visible_size && (particle_baseline || alpha_can_pass));
        // Keep the billboard in the standard spatial vertex path. This gives
        // Forward+ the same view-space position for depth, fog and rasterization.
        MODELVIEW_MATRIX = mat4(vec4(1.0, 0.0, 0.0, 0.0), vec4(0.0, 1.0, 0.0, 0.0),
            vec4(0.0, 0.0, 1.0, 0.0), center);
        VERTEX = vec3(offset * (particle_baseline || alpha_can_pass ? size : 0.0) * vec2(1.0, 0.875), 0.0);
        float frame = float(int(INSTANCE_CUSTOM.z) & 255);
        UV = (UV + vec2(mod(frame, atlas_size.x), floor(frame / atlas_size.x))) / atlas_size;
    }
}

void fragment() {
    if (visible_in_view < 0.5) discard;
    vec2 size = vec2(textureSize(atlas, 0));
    vec2 texel_uv = clamp(UV, vec2(0.5) / size, 1.0 - vec2(0.5) / size);
    bool nearest = false;
    if (nearest_minification) {
        vec2 texel_position = UV * size;
        vec2 dx = dFdx(texel_position), dy = dFdy(texel_position);
        nearest = max(dot(dx, dx), dot(dy, dy)) > 1.0;
    }
    vec4 texel = (nearest ? texelFetch(atlas, ivec2(texel_uv * size), 0) : texture(atlas, texel_uv)) * 255.0;
    vec4 result = texel * tint / 128.0;
    if (texture_function == 1) result = texel;
    if (texture_function >= 2) {
        result.rgb += tint.a;
        result.a = texel.a + (texture_function == 2 ? tint.a : 0.0);
    }
    result = min(floor(result + 0.001), vec4(255.0));
    if (result.a <= (alpha_test ? 48.0 : 0.0)) discard;
    vec3 rgb = result.rgb / 255.0;
    float blend_alpha = min(result.a / 128.0, 1.0);
    float compensation = 1.0;
#ifdef PARTICLE_ADDITIVE
    if (preview_focal > 0.0) {
        // GS adds the alpha-weighted color in bytes. Convert the weighted
        // contribution first so weak sprites have that intensity over black.
        rgb = min(floor(result.rgb * result.a / 128.0), vec3(255.0)) / 255.0;
        compensation = 1.0 / blend_alpha;
    }
#endif
    ALBEDO = compensation * mix(rgb / 12.92, pow((rgb + 0.055) / 1.055, vec3(2.4)),
        greaterThan(rgb, vec3(0.04045)));
#ifndef PARTICLE_CUTOUT
    ALPHA = blend_alpha;
#endif
}
