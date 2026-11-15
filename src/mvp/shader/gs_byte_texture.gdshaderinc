ivec4 paint_texel(sampler2D image, ivec2 point, ivec2 size, int level, int repeat_bits) {
    ivec2 address = clamp(point, ivec2(0), size - 1);
    if ((repeat_bits & 1) != 0) address.x = point.x & (size.x - 1);
    if ((repeat_bits & 2) != 0) address.y = point.y & (size.y - 1);
    return ivec4(texelFetch(image, address, level) * 255.0 + 0.5);
}

ivec4 paint_sample_level(sampler2D image, vec2 uv, int level, bool linear, int repeat_bits) {
    ivec2 size = textureSize(image, level);
    vec2 p = uv * vec2(size);
    if (!linear) return paint_texel(image, ivec2(floor(p)), size, level, repeat_bits);
    p -= vec2(0.5);
    ivec2 i = ivec2(floor(p));
    ivec2 weight = ivec2(fract(p) * 256.0);
    ivec4 a = paint_texel(image, i, size, level, repeat_bits);
    ivec4 b = paint_texel(image, i + ivec2(1, 0), size, level, repeat_bits);
    ivec4 c = paint_texel(image, i + ivec2(0, 1), size, level, repeat_bits);
    ivec4 d = paint_texel(image, i + ivec2(1, 1), size, level, repeat_bits);
    ivec4 top = a + (((b - a) * weight.x) >> 8);
    ivec4 bottom = c + (((d - c) * weight.x) >> 8);
    return top + (((bottom - top) * weight.y) >> 8);
}

vec4 paint_sample(sampler2D image, vec2 uv, ivec4 state, float lod) {
    if (lod <= 0.0) return vec4(paint_sample_level(image, uv, 0, state.y != 0, state.x));
    int mode = state.z;
    bool linear = mode == 1 || mode == 4 || mode == 5;
    if (state.w == 0 || mode < 2) return vec4(paint_sample_level(image, uv, 0, linear, state.x));
    lod = min(lod, float(state.w));
    if (mode == 2 || mode == 4) {
        return vec4(paint_sample_level(image, uv, int(lod + 0.5), linear, state.x));
    }
    int level = int(lod);
    ivec4 a = paint_sample_level(image, uv, level, linear, state.x);
    ivec4 b = paint_sample_level(image, uv, min(level + 1, state.w), linear, state.x);
    return vec4(a + (((b - a) * int(fract(lod) * 256.0)) >> 8));
}

vec4 paint_modulate(vec4 rgba, vec4 texel) {
    return min(floor(rgba * texel / 128.0), vec4(255.0));
}

vec3 gs_byte_rgb(vec3 color, bool srgb) {
    vec3 encoded = mix(color * 12.92, 1.055 * pow(max(color, vec3(0.0)),
        vec3(1.0 / 2.4)) - 0.055, greaterThan(color, vec3(0.0031308)));
    return round((srgb ? color : encoded) * 255.0);
}

vec3 gs_encoded_to_linear(vec3 color) {
    return mix(color / 12.92, pow((color + 0.055) / 1.055, vec3(2.4)), step(vec3(0.04045), color));
}

