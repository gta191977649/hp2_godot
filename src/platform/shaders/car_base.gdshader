shader_type spatial;
render_mode unshaded, cull_disabled, @RENDER_MODES@;
uniform sampler2D base_texture : @SAMPLER_HINT@;
uniform bool base_lit = true;
uniform bool flag_0x4 = false;
uniform bool reflection_pass = false;
// Host suppression of an original unsubmitted strip; default preserves race/viewer paths.
uniform bool frontend_skip_strip = false;
uniform vec3 environment_row0;
uniform vec3 environment_row1;
uniform vec3 environment_row2;
uniform vec3 source_camera = vec3(0.0);
uniform vec4 material_350;
uniform vec4 material_351;
uniform vec4 material_352;
uniform vec4 material_353;
uniform vec4 light_35c;
uniform vec4 light_35d;
uniform vec4 light_35e;
uniform vec4 light_35f;
uniform vec4 light_360;
uniform vec4 light_361;
uniform vec4 light_362;
varying vec4 gs_color_numerator;
varying float gs_color_w;

vec3 hp2_framebuffer_rgb(vec3 encoded){
    // Host adapter, pinned Godot4.4.1 tonemap_inc.glsl: GLES applies a cubic
    // ALBEDO decoder followed by a power encoder; they are not inverses.
    // Invert that cubic so framebuffer bytes retain the original GS RGB.
    if(OUTPUT_IS_SRGB){
        vec3 target = pow((encoded + vec3(0.055)) / 1.055, vec3(2.4));
        vec3 value = encoded;
        for(int iteration=0;iteration<8;iteration++){
            vec3 polynomial = value * (value * (value * 0.305306011 + 0.682171111) + 0.012522878);
            vec3 derivative = value * (value * 0.915918033 + 1.364342222) + 0.012522878;
            value = clamp(value - (polynomial - target) / derivative, vec3(0.0), vec3(1.0));
        }
        return mix(value, vec3(0.0), equal(encoded, vec3(0.0)));
    }
    // TASK0071: RenderingDevice targets keep numeric GS RGB in linear HDR
    // storage (use_hdr_2d). Blend in that domain; no per-fragment gamma decode.
    // HDR does not COLCLAMP after each primitive. Consumers clamp on resolve;
    // overlapping saturation followed by blending remains an explicit host gap.
    return encoded;
}

float vu_bit_affine(float value, float scale, float bias){
    float integer_value = float(floatBitsToInt(value));
    float product = integer_value * scale;
    return intBitsToFloat(int(bias + product));
}
vec3 vu_reflection(vec3 position, vec3 normal){
    vec3 delta = position - source_camera;
    float twice_dot = delta.x * normal.x;
    twice_dot += delta.x * normal.x;
    twice_dot += delta.y * normal.y;
    twice_dot += delta.y * normal.y;
    twice_dot += delta.z * normal.z;
    twice_dot += delta.z * normal.z;
    vec3 reflected = delta - normal * twice_dot;
    float square_length = reflected.x * reflected.x;
    square_length += reflected.y * reflected.y;
    square_length += reflected.z * reflected.z;
    float inverse_length = vu_bit_affine(square_length, -0.5, intBitsToFloat(0x4ebe6189));
    return reflected * inverse_length;
}
vec4 vu_base_color(vec3 normal, vec3 reflected, vec3 packed_rgb){
    vec3 d = light_360.xyz * normal.x;
    d += light_361.xyz * normal.y;
    d += light_362.xyz * normal.z;
    d = max(d, vec3(0.0));
    vec3 s = light_360.xyz * reflected.x;
    s += light_361.xyz * reflected.y;
    s += light_362.xyz * reflected.z;
    s = max(s, vec3(material_350.z));
    vec2 power_coefficients = flag_0x4 ? material_353.xy : material_350.xy;
    s = vec3(vu_bit_affine(s.x, power_coefficients.x, power_coefficients.y),
             vu_bit_affine(s.y, power_coefficients.x, power_coefficients.y),
             vu_bit_affine(s.z, power_coefficients.x, power_coefficients.y));
    vec4 value = vec4(light_35f.rgb, flag_0x4 ? material_353.z : light_35f.a);
    vec4 d0 = vec4(light_35c.rgb * material_351.rgb, flag_0x4 ? 0.0 : light_35c.a);
    vec4 d1 = vec4(light_35d.rgb * material_351.rgb, flag_0x4 ? 0.0 : light_35d.a);
    vec4 d2 = vec4(light_35e.rgb * material_351.rgb, flag_0x4 ? 0.0 : light_35e.a);
    value += d0 * d.x;
    value += d1 * d.y;
    value += d2 * d.z;
    vec4 s0 = vec4(light_35c.rgb * material_352.rgb, flag_0x4 ? material_353.w : 0.0);
    vec4 s1 = vec4(light_35d.rgb * material_352.rgb, 0.0);
    vec4 s2 = vec4(light_35e.rgb * material_352.rgb, 0.0);
    value += s0 * s.x;
    value += s1 * s.y;
    value += s2 * s.z;
    value.rgb *= vec3(1.0) + (packed_rgb - vec3(255.0)) * material_350.w;
    return vec4(ivec4(clamp(value, vec4(0.0), vec4(255.0))));
}
void vertex(){
    // Inverse of the bridge's orientation-preserving (x,y,z)->(y,z,x).
    vec3 source_position = vec3(VERTEX.z, VERTEX.x, VERTEX.y);
    // CUSTOM0 bypasses Godot's packed/normalized normal vertex format.
    vec3 source_normal = CUSTOM0.xyz;
    vec4 packed = round(COLOR * 255.0);
    vec4 gs_rgba;
    if(base_lit){
        gs_rgba = vu_base_color(source_normal, vu_reflection(source_position, source_normal), packed.rgb);
    }else{
        gs_rgba = packed;
    }
    if(reflection_pass){
        // E-0035-R03, original VU42B..4B3. Context2 uses the second material block.
        vec3 reflected = vu_reflection(source_position, source_normal);
        vec3 direction = reflected.x * environment_row0;
        direction += reflected.y * environment_row1;
        direction += reflected.z * environment_row2;
        direction.xz = -direction.xz;
        float z = direction.z + 1.0;
        float square_length = direction.x * direction.x + direction.y * direction.y;
        square_length += z * z;
        float inverse_length = vu_bit_affine(square_length, -0.5, intBitsToFloat(0x4ebe6189));
        UV = vec2(0.5) + (direction.xy * inverse_length) * 0.5;
        float reflection_alpha = float(int(material_351.a + material_352.a * direction.z));
        gs_rgba = vec4(flag_0x4 ? gs_rgba.rgb : vec3(128.0), reflection_alpha);
    }
    // GS Gouraud color is linear in screen space; ST/Q remains perspective.
    gs_color_w = (PROJECTION_MATRIX * MODELVIEW_MATRIX * vec4(VERTEX, 1.0)).w;
    gs_color_numerator = gs_rgba * gs_color_w;
}
void fragment(){
if(frontend_skip_strip){discard;}
vec4 gs_rgba = gs_color_numerator / gs_color_w;
@FRAGMENT_SOURCE@
}
