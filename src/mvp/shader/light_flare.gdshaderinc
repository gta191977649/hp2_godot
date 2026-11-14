uniform sampler2D flare : source_color, filter_linear, repeat_disable;
instance uniform vec4 normal_and_sides;
instance uniform vec4 response;
instance uniform vec4 distance_scale;
instance uniform vec4 minimum_color;
instance uniform vec4 maximum_color;
varying vec4 tint;

void vertex() {
    vec3 center = MODEL_MATRIX[3].xyz;
    vec3 to_camera = normalize(INV_VIEW_MATRIX[3].xyz - center);
    float facing = dot(normal_and_sides.xyz, to_camera);
    if (normal_and_sides.w > 0.5) facing = abs(facing);
    float blend = pow(max(facing, 0.01), response.z) * response.w;
    vec4 view_center = VIEW_MATRIX * vec4(center, 1.0);
    float focal = 240.0 * PROJECTION_MATRIX[1][1];
    float size = mix(response.x, response.y, blend) *
        clamp(-view_center.z / focal, distance_scale.x, distance_scale.y);
    vec4 biased = VIEW_MATRIX * vec4(center + to_camera * distance_scale.z, 1.0);
    POSITION = PROJECTION_MATRIX * (biased + vec4(VERTEX.xy * 2.0 * size, 0.0, 0.0));
    tint = mix(minimum_color, maximum_color, blend);
}

void fragment() {
    vec4 texel = texture(flare, UV);
    ALBEDO = texel.rgb * tint.rgb;
    ALPHA = clamp(texel.a * tint.a, 0.0, 1.0);
}
