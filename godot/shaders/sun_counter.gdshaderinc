// Port-side transport of StuffSun Retail001252B0 / Debug00125430 alpha.
// The original count affects alpha only; keep multiplication order and low byte.
int hp2_sun_count(vec4 encoded) {
    ivec2 bytes = ivec2(floor(encoded.rg * 255.0 + vec2(0.5)));
    return bytes.x + bytes.y * 256;
}
float hp2_sun_alpha(int count, float intensity) {
    float coverage = float(count) * (1.0 / 1024.0);
    float alpha = intensity < 0.0 ? -intensity * 128.0 : (coverage * intensity) * 128.0;
    return float(int(alpha) & 255);
}

