shader_type canvas_item;
render_mode unshaded;
varying float texture_q;
varying vec4 vertex_color;
void vertex() {
    texture_q = CUSTOM0.x;
    vertex_color = COLOR;
}
void fragment() {
    COLOR = vertex_color * texture(TEXTURE, UV / texture_q);
}
