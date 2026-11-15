shader_type canvas_item;
render_mode blend_premul_alpha, unshaded;

varying vec4 modulation;

void vertex() {
    modulation = COLOR;
}

void fragment() {
    // The 3D target already blended its translucent pixels against clear.
    vec4 preview = texture(TEXTURE, UV);
    COLOR = preview * vec4(modulation.rgb, 1.0) * modulation.a;
}
