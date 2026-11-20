uniform sampler2D base_image : filter_linear_mipmap, repeat_enable;
uniform sampler2D specular_image : filter_linear_mipmap, repeat_enable;
uniform sampler2D reflection_image : filter_linear_mipmap, repeat_enable;
uniform int paint_flags;
uniform bool alpha_test;
uniform bool frontend;
uniform bool wheel;
uniform ivec2 repeat_axes;
uniform vec4 coefficients[4];
uniform ivec4 reflection_selectors;
uniform float reflection_fixed;
uniform vec4 frontend_power;
// TEX0 repeat bits, MMAG, MMIN and MXL for each authored texture.
uniform ivec4 base_sampling;
uniform ivec4 specular_sampling;
uniform ivec4 reflection_sampling;
uniform float base_lod_scale;
uniform int base_tfx;
#ifdef CAR_GLASS
uniform sampler2D opaque_scene : hint_screen_texture, filter_nearest, repeat_disable;
#endif
instance uniform vec4 hp2_paint_light_r;
instance uniform vec4 hp2_paint_light_g;
instance uniform vec4 hp2_paint_light_b;
instance uniform vec4 hp2_paint_light_a;
instance uniform vec3 hp2_paint_direction_0;
instance uniform vec3 hp2_paint_direction_1;
instance uniform vec3 hp2_paint_direction_2;
instance uniform vec3 hp2_paint_env_x;
instance uniform vec3 hp2_paint_env_y;
instance uniform vec3 hp2_paint_env_z;
instance uniform vec3 hp2_paint_eye;
instance uniform bool hp2_paint_bound;
instance uniform vec3 hp2_paint_focal;

varying vec4 base_weighted;
varying vec2 reflection_uv;
varying float reflection_alpha_weighted;
varying float clip_weight;
varying flat float reflection_on;
varying flat float base_mip_bias;

float vu_power(float value, vec2 parameters) {
    return intBitsToFloat(int(float(floatBitsToInt(value)) * parameters.x + parameters.y));
}

float inverse_root(float squared) {
    if (frontend) {
        return vu_power(squared, vec2(-0.5, intBitsToFloat(0x4ebe6189)));
    }
    return inversesqrt(max(squared, 1e-12));
}

float response(float value, bool alpha) {
    if (frontend) return vu_power(max(coefficients[0].z, value),
        alpha ? frontend_power.zw : frontend_power.xy);
    vec4 c = alpha ? coefficients[3] : coefficients[0];
    if (c.y == 0.0) return 0.0;
    return c.y * pow(max(coefficients[0].z, value), c.x);
}

void vertex() {
    vec3 position = vec3(VERTEX.x, -VERTEX.z, VERTEX.y);
    vec3 normal = CUSTOM0.xyz;
    vec4 authored = vec4(CUSTOM0.w);
    authored.rgb = vec3(float(int(CUSTOM0.w) & 31),
        float((int(CUSTOM0.w) >> 5) & 31), float((int(CUSTOM0.w) >> 10) & 31)) * 8.0;
    authored.a = frontend ? float((int(CUSTOM0.w) >> 15) & 1) * 128.0 : 128.0;
    base_mip_bias = CUSTOM2.x;
    vec3 render_eye = (inverse(MODELVIEW_MATRIX) * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    vec3 source_eye = frontend ? hp2_paint_eye : vec3(render_eye.x, -render_eye.z, render_eye.y);
    vec3 eye_ray = position - source_eye;
    vec3 ray = reflect(eye_ray, normal);
    ray *= inverse_root(dot(ray, ray));
    vec3 mapped = vec3(-dot(hp2_paint_env_x, ray), dot(hp2_paint_env_y, ray), -dot(hp2_paint_env_z, ray));
    if (wheel && !frontend) {
        vec3 view_ray = mat3(MODELVIEW_MATRIX) * vec3(ray.x, ray.z, -ray.y);
        mapped = -view_ray;
    }
    vec3 sphere = mapped + vec3(0.0, 0.0, 1.0);
    reflection_uv = vec2(0.5) + 0.5 * sphere.xy * inverse_root(dot(sphere, sphere));
    float reflection_alpha = float(int(coefficients[1].w + coefficients[2].w * mapped.z) & 255);
    clip_weight = (PROJECTION_MATRIX * MODELVIEW_MATRIX * vec4(VERTEX, 1.0)).w;
    reflection_alpha_weighted = reflection_alpha * clip_weight;
    vec3 view_origin = MODELVIEW_MATRIX[3].xyz;
    float focal = abs(PROJECTION_MATRIX[1][1]) > hp2_paint_focal.z ? hp2_paint_focal.y : hp2_paint_focal.x;
    float distance = length(view_origin) - 3.0;
    float pixels = distance > 1.5 ? 3.0 * focal / distance : 2.0 * focal;
    reflection_on = frontend || (view_origin.z <= 3.0 && trunc(pixels) >= 18.0) ? 1.0 : 0.0;
    if (hp2_paint_bound && (paint_flags & 128) != 0) {
        vec3 diffuse = max(vec3(dot(hp2_paint_direction_0, normal), dot(hp2_paint_direction_1, normal),
            dot(hp2_paint_direction_2, normal)), vec3(0.0));
        vec3 angles = vec3(dot(hp2_paint_direction_0, ray), dot(hp2_paint_direction_1, ray),
            dot(hp2_paint_direction_2, ray));
        bool material_alpha = (paint_flags & 4) != 0;
        vec3 specular = vec3(response(angles.x, material_alpha), response(angles.y, material_alpha),
            response(angles.z, material_alpha));
        vec3 diffuse_color = vec3(dot(hp2_paint_light_r.xyz, diffuse),
            dot(hp2_paint_light_g.xyz, diffuse), dot(hp2_paint_light_b.xyz, diffuse));
        vec3 highlight = vec3(dot(hp2_paint_light_r.xyz, specular),
            dot(hp2_paint_light_g.xyz, specular), dot(hp2_paint_light_b.xyz, specular));
        vec3 ambient = vec3(hp2_paint_light_r.w, hp2_paint_light_g.w, hp2_paint_light_b.w);
        authored.rgb = trunc(clamp((ambient + diffuse_color * coefficients[1].xyz +
            highlight * coefficients[2].xyz) * (1.0 + (authored.rgb - 255.0) * coefficients[0].w), 0.0, 255.0));
        authored.a = material_alpha ? coefficients[3].z + coefficients[3].w * specular.x :
            hp2_paint_light_a.w + dot(hp2_paint_light_a.xyz, diffuse);
    }
    if ((paint_flags & 64) != 0) {
        authored.a = coefficients[3].z;
        if (coefficients[3].w != 0.0)
            authored.a += coefficients[3].w * response(dot(hp2_paint_direction_0, ray), true);
    }
    authored.a = trunc(clamp(authored.a, 0.0, 255.0));
    base_weighted = authored * clip_weight;
}

vec3 operand(int selector, vec3 source, vec3 destination) {
    return selector == 0 ? source : selector == 1 ? destination : vec3(0.0);
}

vec4 linear_clamped(sampler2D image, vec2 uv) {
    vec2 edge = 0.5 / vec2(textureSize(image, 0));
    return textureLod(image, clamp(uv, edge, 1.0 - edge), 0.0);
}

#include "gs_byte_texture.gdshaderinc"

vec4 frontend_base(vec4 rgba, vec2 uv, float lod) {
    vec4 texel = paint_sample(base_image, uv, base_sampling, lod * base_lod_scale + base_mip_bias);
    if (base_tfx == 1) return texel;
    vec4 result = paint_modulate(rgba, texel);
    if (base_tfx >= 2) {
        result.rgb = min(result.rgb + rgba.a, vec3(255.0));
        result.a = base_tfx == 2 ? min(texel.a + rgba.a, 255.0) : texel.a;
    }
    return result;
}

vec3 frontend_highlights(vec4 rgba, vec2 uv, float lod, float destination_alpha) {
    vec3 highlights = vec3(0.0);
    if ((paint_flags & 64) != 0) {
        vec4 gloss = paint_modulate(rgba, paint_sample(specular_image, uv, specular_sampling, lod));
        if (!alpha_test || gloss.a > 48.0) highlights += floor(gloss.rgb * destination_alpha / 128.0);
    }
    if (hp2_paint_bound && (paint_flags & 256) != 0) {
        // Frontend VU slots 0x318 and 0x31c use the map's own source alpha.
        float alpha = floor(reflection_alpha_weighted / clip_weight + 0.01);
        vec3 tint = (paint_flags & 4) != 0 ? rgba.rgb : vec3(128.0);
        vec4 reflected = paint_modulate(vec4(tint, alpha),
            paint_sample(reflection_image, reflection_uv, reflection_sampling, 0.0));
        // Retail TEST_2 keeps Z testing active with alpha testing off.
        highlights += floor(reflected.rgb * reflected.a / 128.0);
    }
    return highlights;
}

vec3 race_paint(vec4 base, vec2 uv, vec3 behind, out float opacity) {
    vec2 half_texel = 0.5 / vec2(textureSize(base_image, 0));
    uv = mix(clamp(uv, half_texel, 1.0 - half_texel), uv, notEqual(repeat_axes, ivec2(0)));
    vec4 texel = texture(base_image, uv);
    float destination_alpha = min(255.0, floor(texel.a * 255.0 * base.a / 128.0));
    opacity = min(destination_alpha / 128.0, 1.0);
    if (alpha_test && destination_alpha <= 48.0) discard;
    vec3 paint = clamp(floor(texel.rgb * 255.0 * base.rgb / 128.0), 0.0, 255.0);
#ifdef CAR_GLASS
    // GS ALPHA 0x44 blends WINDOW into the destination before the
    // independent context-2 reflection uses the resulting RGB and base alpha.
    paint = clamp(behind + floor((paint - behind) * destination_alpha / 128.0), 0.0, 255.0);
#endif
    if ((paint_flags & 64) != 0) {
        vec4 falloff = linear_clamped(specular_image, uv);
        vec3 gloss = floor(falloff.rgb * 255.0 * base.rgb / 128.0);
        paint = clamp(paint + floor(gloss * destination_alpha / 128.0), 0.0, 255.0);
        // The specular packet writes its source alpha for the next context.
        destination_alpha = min(255.0, floor(falloff.a * 255.0 * base.a / 128.0));
    }
    if (hp2_paint_bound && reflection_on > 0.0 && (paint_flags & 256) != 0) {
        vec4 env = linear_clamped(reflection_image, reflection_uv);
        vec3 reflect_rgb = (paint_flags & 4) != 0 ? base.rgb : vec3(128.0);
        vec3 source = clamp(floor(env.rgb * 255.0 * reflect_rgb / 128.0), 0.0, 255.0);
        float source_alpha = floor(env.a * 255.0 * reflection_alpha_weighted / (128.0 * clip_weight));
        float weight = reflection_selectors.z == 0 ? source_alpha :
            reflection_selectors.z == 1 ? destination_alpha : reflection_fixed;
        paint = clamp(operand(reflection_selectors.w, source, paint) +
            floor((operand(reflection_selectors.x, source, paint) -
            operand(reflection_selectors.y, source, paint)) * weight / 128.0), 0.0, 255.0);
    }
    return paint / 255.0;
}

void fragment() {
    vec4 base = base_weighted / clip_weight;
    if (frontend) {
        vec4 rgba = floor(base + 0.01);
        float lod = log2(max(clip_weight / hp2_paint_focal.x, 1e-12));
        vec4 paint = frontend_base(rgba, UV, lod);
        if (alpha_test && paint.a <= 48.0) discard;
        vec3 highlights = frontend_highlights(rgba, UV, lod, paint.a);
#ifdef CAR_GLASS
        vec4 backdrop = texture(opaque_scene, SCREEN_UV);
        float opacity = min(paint.a / 128.0, 1.0);
        // The WINDOW texture lerps in GS byte space before the environment kick.
        vec3 destination = backdrop.a >= 0.999 ? gs_byte_rgb(backdrop.rgb, OUTPUT_IS_SRGB) : vec3(0.0);
        vec3 encoded = clamp(destination + floor((paint.rgb - destination) * paint.a / 128.0) +
            highlights, vec3(0.0), vec3(255.0)) / 255.0;
        ALPHA = backdrop.a >= 0.999 ? 1.0 : opacity;
#else
        vec3 encoded = min(paint.rgb + highlights, vec3(255.0)) / 255.0;
#endif
        ALBEDO = OUTPUT_IS_SRGB ? encoded : gs_encoded_to_linear(encoded);
    } else {
        vec3 behind = vec3(0.0);
#ifdef CAR_GLASS
        vec3 backdrop = texture(opaque_scene, SCREEN_UV).rgb;
        behind = gs_byte_rgb(backdrop, OUTPUT_IS_SRGB);
#endif
        float opacity;
        vec3 encoded = race_paint(base, UV, behind, opacity);
        ALBEDO = OUTPUT_IS_SRGB ? encoded : gs_encoded_to_linear(encoded);
#ifdef CAR_GLASS
        // The screen copy contains opaque geometry only. Keep the authored
        // byte-space result over that reference, but let the blend unit retain
        // earlier transparent draws (smoke, flames, flares and other windows).
        // An opaque output would replace all of them with the screen copy.
        // Byte-space darkening can be stronger than linear alpha blending.
        // Increase coverage just enough to keep the premultiplied contribution
        // nonnegative while reproducing the original no-particle glass color.
        vec3 retained = clamp(ALBEDO / max(backdrop, vec3(1e-6)), vec3(0.0), vec3(1.0));
        float transmission = min(1.0 - opacity, min(retained.r, min(retained.g, retained.b)));
        ALBEDO = max(ALBEDO - backdrop * transmission, vec3(0.0));
        ALPHA = 1.0 - transmission;
#endif
    }
}
