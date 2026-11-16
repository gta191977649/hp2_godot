shader_type canvas_item;
render_mode unshaded, blend_disabled;

#include "gs_byte_texture.gdshaderinc"

void fragment() {
    vec3 color = clamp(texture(TEXTURE, UV).rgb, vec3(0.0), vec3(1.0));
    // The 3D faces are linear HDR. The atlas carries GS color bytes in
    // an HDR texture so the vehicle shader can sample those bytes directly.
    vec3 face_bytes = gs_byte_rgb(color, false);
    COLOR = vec4(floor(face_bytes * 96.0 / 128.0) / 255.0, 64.0 / 255.0);
}
