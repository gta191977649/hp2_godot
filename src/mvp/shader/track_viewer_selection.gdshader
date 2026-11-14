shader_type spatial;
render_mode unshaded, cull_disabled, shadows_disabled, fog_disabled;

void vertex() {
    POSITION = PROJECTION_MATRIX * MODELVIEW_MATRIX * vec4(VERTEX, 1.0);
    // Keep the selected mesh in front of its batched copy with reverse-Z.
    POSITION.z += 0.000002 * POSITION.w;
}

void fragment() {
    ALBEDO = vec3(1.0, 0.025, 0.025);
}
