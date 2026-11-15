// Port-side material path for the original GS track overlays.
uniform sampler2D gs_texture_linear : filter_linear_mipmap, repeat_enable;
uniform sampler2D gs_texture_nearest : filter_nearest_mipmap, repeat_enable;
uniform sampler2DArray gs_frames_linear : filter_linear_mipmap, repeat_enable;
uniform sampler2DArray gs_frames_nearest : filter_nearest_mipmap, repeat_enable;
uniform sampler2D gs_specular : filter_linear, repeat_enable;
uniform ivec4 gs_filter;
uniform bool gs_test;
uniform bool gs_copy;
uniform bool gs_dark_blend;
uniform bool gs_road;
global uniform bool hp2_roadshine;
global uniform float hp2_track_focal_scale;
uniform vec4 gs_road_alpha;
uniform float gs_road_min_dot;
uniform vec3 gs_sun;
uniform int gs_bias;

varying vec4 gs_color_weighted;
varying float gs_clip_weight;
varying flat float gs_mip_bias;

#if defined(GS_ROAD_WATER) && defined(GS_MIX)
uniform bool gs_has_wet_mask;
uniform sampler2D gs_wet_mask_linear : filter_linear, repeat_enable;
uniform sampler2D gs_wet_mask_nearest : filter_nearest, repeat_enable;
uniform ivec2 gs_wet_mask_repeat;
varying vec4 gs_wet_mask_color_weighted;
varying flat float gs_wet_mask_mip_bias;
#endif

vec4 gs_vertex_color(vec3 point, vec4 color, vec3 native_normal, mat4 modelview, mat4 model,
        mat4 inverse_view, bool road) {
    vec4 rgba = round(color * 255.0);
    if (road) {
        vec3 normal = vec3(native_normal.x, native_normal.z, -native_normal.y);
        vec3 eye = (inverse(modelview) * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
        // Identity scenery uses the original world light context (PAL107900).
        bool world = all(equal(model[3].xyz, vec3(0.0))) &&
            all(equal(model[0].xyz, vec3(1.0, 0.0, 0.0))) &&
            all(equal(model[1].xyz, vec3(0.0, 1.0, 0.0))) &&
            all(equal(model[2].xyz, vec3(0.0, 0.0, 1.0)));
        if (world) eye += inverse_view[2].xyz * 10.0 + vec3(0.0, 4.0, 0.0);
        vec3 ray = reflect(point - eye, normal);
        float length_squared = dot(ray, ray);
        if (length_squared > 0.0)
            ray *= inversesqrt(length_squared);
        vec3 light = normalize(transpose(mat3(model)) * (gs_sun - model[3].xyz));
        // EcstasyLitVertexColor uses decoded exponent/amplitude, not the VU's packed bias.
        float power = gs_road_alpha.y * pow(max(dot(ray, light), gs_road_min_dot), gs_road_alpha.x);
        rgba.a = clamp(trunc(gs_road_alpha.z + gs_road_alpha.w * power), 0.0, 255.0);
    }
    return rgba;
}

vec2 gs_uv(vec2 uv, vec2 size, ivec2 axes) {
    vec2 edge = 0.5 / size;
    return mix(clamp(uv, edge, 1.0 - edge), uv, notEqual(axes, ivec2(0)));
}

vec4 gs_sample(vec2 uv, int level, bool linear, float frame) {
    vec2 size = vec2(textureSize(gs_texture_linear, level));
    vec2 p = gs_uv(uv, size, repeat_axes);
    if (texture_count > 0) {
        return linear ? textureLod(gs_frames_linear, vec3(p, frame), float(level)) :
            textureLod(gs_frames_nearest, vec3(p, frame), float(level));
    }
    return linear ? textureLod(gs_texture_linear, p, float(level)) :
        textureLod(gs_texture_nearest, p, float(level));
}

vec4 gs_sample_base(vec2 uv, float q) {
    float lod = -log2(max(q, 1e-12)) * float(1 << gs_filter.w) + gs_mip_bias;
    float frame = 0.0;
    if (texture_count > 0) {
        float clock = real_clock ? hp2_animation_clock.z : hp2_animation_clock.w;
        frame = mod(trunc(texture_rate * (clock / 60.0)), float(texture_count));
    }
    int mode = gs_filter.z;
    if (lod <= 0.0) return gs_sample(uv, 0, gs_filter.y != 0, frame);
    if (mode < 2 || gs_filter.x == 0) return gs_sample(uv, 0, mode == 1 || mode >= 4, frame);
    lod = clamp(lod, 0.0, float(gs_filter.x));
    bool linear = mode >= 4;
    if ((mode & 1) == 0) return gs_sample(uv, int(floor(lod + 0.5)), linear, frame);
    int level = int(floor(lod));
    return mix(gs_sample(uv, level, linear, frame),
        gs_sample(uv, min(level + 1, gs_filter.x), linear, frame), fract(lod));
}

vec3 gs_to_linear(vec3 color) {
    return mix(color / 12.92, pow((color + 0.055) / 1.055, vec3(2.4)), greaterThan(color, vec3(0.04045)));
}

vec3 gs_road_specular(vec2 uv, vec3 tint, float alpha) {
    vec3 specular = floor(textureLod(gs_specular, uv, 0.0).rgb * 255.0 + 0.001);
    specular = min(floor(specular * tint / 128.0 + 0.001), vec3(255.0));
    return floor(specular * alpha);
}

vec4 gs_fragment(vec2 uv, bool srgb, float q) {
    vec4 rgba = gs_color_weighted / gs_clip_weight;
    vec4 texel = floor(gs_sample_base(uv, q) * 255.0 + 0.001);
    vec4 source = min(floor(texel * rgba / 128.0 + 0.001), vec4(255.0));
    if (gs_test && source.a <= 48.0) discard;
    float alpha = source.a / 128.0;
    vec3 color = source.rgb;
#if defined(GS_ADD) || defined(GS_SUB)
    color = floor(color * alpha);
#endif
    if (gs_road && hp2_roadshine) color += gs_road_specular(uv, rgba.rgb, alpha);
    vec3 encoded = clamp(color, vec3(0.0), vec3(255.0)) / 255.0;
#ifdef GS_ROAD_WATER
    return vec4(encoded, alpha);
#else
    vec3 linear = srgb ? encoded : gs_to_linear(encoded);
#if defined(GS_ADD) || defined(GS_SUB)
    return vec4(linear, 1.0);
#elif defined(GS_MIX)
    float weight = gs_copy ? 1.0 : clamp(alpha, 0.0, 1.0);
    if (!srgb && gs_dark_blend) {
        // Dark-destination equivalent, fitted in luminance to preserve tint.
        vec3 luma = vec3(0.2126, 0.7152, 0.0722);
        float source_luma = dot(linear, luma);
        if (source_luma > 0.00001)
            weight = dot(gs_to_linear(encoded * weight), luma) / source_luma;
    }
    return vec4(linear, weight);
#else
    return vec4(linear, 1.0);
#endif
#endif
}

#ifdef GS_ROAD_WATER
uniform sampler2D gs_road_background : hint_screen_texture, filter_nearest, repeat_disable;
#ifdef GS_MIX
uniform sampler2D gs_road_depth : hint_depth_texture, filter_nearest, repeat_disable;
uniform sampler2D gs_water_sky : filter_linear, repeat_disable;
uniform bool gs_has_water_sky;

vec3 gs_wet_mask(vec2 uv, float q) {
    if (!gs_has_wet_mask) return vec3(0.0);
    vec2 p = gs_uv(uv, vec2(textureSize(gs_wet_mask_linear, 0)), gs_wet_mask_repeat);
    float lod = -log2(max(q, 1e-12)) + gs_wet_mask_mip_bias;
    vec4 sample_color = lod <= 0.0 ? textureLod(gs_wet_mask_linear, p, 0.0) :
        textureLod(gs_wet_mask_nearest, p, 0.0);
    vec4 rgba = gs_wet_mask_color_weighted / gs_clip_weight;
    vec4 texel = floor(sample_color * 255.0 + 0.001);
    vec4 source = min(floor(texel * rgba / 128.0 + 0.001), vec4(255.0));
    float alpha = source.a / 128.0;
    vec3 contribution = floor(source.rgb * alpha);
    if (hp2_roadshine) contribution += gs_road_specular(uv, rgba.rgb, alpha);
    return contribution;
}
#endif

vec4 gs_water_blend(vec4 source, vec2 uv, float surface_depth, bool srgb, bool mirror,
        vec2 material_uv, float q) {
#ifdef GS_MIX
    float alpha = min(source.a, 1.0);
    vec3 highlight = gs_wet_mask(material_uv, q);
    float background_depth = textureLod(gs_road_depth, uv, 0.0).r;
    bool has_road = background_depth != 0.0 &&
        abs(background_depth - surface_depth) <= max(fwidth(surface_depth), 1e-6);
    if (!has_road && gs_has_water_sky && !mirror) {
        // The original draw order puts sky behind this strip. The opaque
        // snapshot can already contain distant scenery through the road.
        vec3 sky = round(textureLod(gs_water_sky, uv, 0.0).rgb * 255.0);
        vec3 wet = clamp(sky + floor((round(source.rgb * 255.0) - sky) * alpha) + highlight,
            vec3(0.0), vec3(255.0)) / 255.0;
        return vec4(srgb ? wet : gs_to_linear(wet), 1.0);
    }
    // The original sky is drawn before the wet asphalt. Godot draws it in
    // the transparent queue, so it is absent from the opaque screen copy.
    // Keep the authored source alpha over that live framebuffer color.
    if (alpha == 0.0 || background_depth == 0.0) {
        // The paired retail underlay has nonzero alpha. Use its premultiplied
        // RGB term when a camera can only blend over the live sky.
        vec3 wet = clamp(source.rgb + highlight / (255.0 * max(alpha, 1e-6)),
            vec3(0.0), vec3(1.0));
        return vec4(srgb ? wet : gs_to_linear(wet), alpha);
    }
#endif
    vec3 background = textureLod(gs_road_background, uv, 0.0).rgb;
    vec3 encoded = srgb ? background : mix(background * 12.92,
        1.055 * pow(max(background, vec3(0.0)), vec3(1.0 / 2.4)) - 0.055,
        greaterThan(background, vec3(0.0031308)));
    vec3 destination = round(clamp(encoded, vec3(0.0), vec3(1.0)) * 255.0);
    vec3 contribution = round(source.rgb * 255.0);
#ifdef GS_MIX
    // ALPHA 0x44: the underlay blends over the opaque road. GS rounds down.
    vec3 combined = clamp(destination + floor((contribution - destination) * alpha) + highlight,
        vec3(0.0), vec3(255.0)) / 255.0;
    vec3 blended = srgb ? combined : gs_to_linear(combined);
    // Convert the source term to Forward+ units while retaining source alpha.
    return vec4((blended - background * (1.0 - alpha)) / alpha, alpha);
#else
    // ALPHA 0x48: add the linear difference for the GS byte increment at
    // this road brightness. Fixed-function add preserves the earlier underlay.
    vec3 combined = (destination + contribution) / 255.0;
    vec3 base = destination / 255.0;
    return vec4(srgb ? combined - base : gs_to_linear(combined) - gs_to_linear(base), 1.0);
#endif
}
#endif

float gs_depth(float z, mat4 projection, float far_clip) {
    // Retail Z24 matrix packs; align pack 0 to Godot's own camera depth.
    float a = -projection[2][2];
    float b = projection[3][2];
    if (far_clip < 0.0) { a = a * 0.5 + 0.5; b *= 0.5; }
    float near = b / (1.0 - a);
    float far = b / -a;
    vec2 base = vec2(intBitsToFloat(0x40000010), intBitsToFloat(0x40800000));
    vec2 range = gs_bias == 1 ? vec2(intBitsToFloat(0x40000040), intBitsToFloat(0x40802000)) :
        vec2(intBitsToFloat(0x40900000), intBitsToFloat(0x40f00000));
    vec2 diff = range - base;
    float reciprocal = 1.0 / z;
    float delta = (diff.x * far - diff.y * near) / (far - near);
    float slope = near * far * (diff.y - diff.x) / (far - near);
    float base_slope = near * far * (base.y - base.x) / (far - near);
    return clamp(a + b * (reciprocal + (delta + slope * reciprocal) / base_slope), 0.0, 1.0);
}
