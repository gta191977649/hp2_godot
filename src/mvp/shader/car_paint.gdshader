shader_type spatial;
render_mode unshaded, cull_disabled;

uniform sampler2D base_image : filter_linear_mipmap, repeat_enable;
uniform sampler2D specular_image : filter_linear, repeat_disable;
uniform sampler2D reflection_image : filter_linear, repeat_disable;
uniform int paint_flags;
uniform bool alpha_test;
uniform bool frontend;
uniform bool wheel;
uniform ivec2 repeat_axes;
uniform vec4 coefficients[4];
uniform ivec4 reflection_selectors;
uniform float reflection_fixed;
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

float response(float value, bool alpha) {
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
    authored.a = 128.0;
    vec3 render_eye = (inverse(MODELVIEW_MATRIX) * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    vec3 source_eye = frontend ? hp2_paint_eye : vec3(render_eye.x, -render_eye.z, render_eye.y);
    vec3 eye_ray = position - source_eye;
    vec3 ray = reflect(eye_ray, normal);
    ray *= inversesqrt(max(dot(ray, ray), 1e-12));
    vec3 mapped = vec3(-dot(hp2_paint_env_x, ray), dot(hp2_paint_env_y, ray), -dot(hp2_paint_env_z, ray));
    if (wheel && !frontend) {
        vec3 view_ray = mat3(MODELVIEW_MATRIX) * vec3(ray.x, ray.z, -ray.y);
        mapped = -view_ray;
    }
    vec3 sphere = mapped + vec3(0.0, 0.0, 1.0);
    reflection_uv = vec2(0.5) + 0.5 * sphere.xy * inversesqrt(max(dot(sphere, sphere), 1e-12));
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

vec3 to_linear(vec3 color) {
    return mix(color / 12.92, pow((color + 0.055) / 1.055, vec3(2.4)), step(vec3(0.04045), color));
}

vec3 operand(int selector, vec3 source, vec3 destination) {
    return selector == 0 ? source : selector == 1 ? destination : vec3(0.0);
}

void fragment() {
    vec4 base = base_weighted / clip_weight;
    vec2 half_texel = 0.5 / vec2(textureSize(base_image, 0));
    vec2 uv = mix(clamp(UV, half_texel, 1.0 - half_texel), UV, notEqual(repeat_axes, ivec2(0)));
    vec4 texel = texture(base_image, uv);
    float destination_alpha = min(255.0, floor(texel.a * 255.0 * base.a / 128.0));
    if (alpha_test && destination_alpha <= 48.0) discard;
    vec3 paint = clamp(floor(texel.rgb * 255.0 * base.rgb / 128.0), 0.0, 255.0);
    if ((paint_flags & 64) != 0) {
        vec3 gloss = floor(texture(specular_image, uv).rgb * 255.0 * base.rgb / 128.0);
        paint = clamp(paint + floor(gloss * destination_alpha / 128.0), 0.0, 255.0);
        // The specular packet writes its source alpha for the next context.
        destination_alpha = min(255.0, floor(texture(specular_image, uv).a * 255.0 * base.a / 128.0));
    }
    if (hp2_paint_bound && reflection_on > 0.0 && (paint_flags & 256) != 0) {
        vec4 env = texture(reflection_image, reflection_uv);
        vec3 reflect_rgb = (paint_flags & 4) != 0 ? base.rgb : vec3(128.0);
        vec3 source = clamp(floor(env.rgb * 255.0 * reflect_rgb / 128.0), 0.0, 255.0);
        float source_alpha = floor(env.a * 255.0 * reflection_alpha_weighted / (128.0 * clip_weight));
        float weight = reflection_selectors.z == 0 ? source_alpha :
            reflection_selectors.z == 1 ? destination_alpha : reflection_fixed;
        paint = clamp(operand(reflection_selectors.w, source, paint) +
            floor((operand(reflection_selectors.x, source, paint) -
            operand(reflection_selectors.y, source, paint)) * weight / 128.0), 0.0, 255.0);
    }
    ALBEDO = OUTPUT_IS_SRGB ? paint / 255.0 : to_linear(paint / 255.0);
}
