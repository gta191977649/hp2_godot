shader_type spatial;
render_mode unshaded, fog_disabled, blend_mix, cull_disabled, depth_draw_never, depth_test_disabled, shadows_disabled;
uniform sampler2D scene_color : hint_screen_texture, filter_nearest, repeat_disable;
uniform sampler2D sky_color : filter_nearest, repeat_disable;
uniform bool has_sky;
uniform sampler2D depth_texture : hint_depth_texture, filter_nearest, repeat_disable;
uniform sampler2D flare_0 : filter_linear, repeat_disable;
uniform sampler2D flare_1 : filter_linear, repeat_disable;
uniform sampler2D flare_2 : filter_linear, repeat_disable;
uniform sampler2D flare_3 : filter_linear, repeat_disable;
uniform sampler2D flare_4 : filter_linear, repeat_disable;
uniform vec4 layer_rect[12]; // UV center, UV half extents
uniform vec4 layer_color[12]; // raw RGBA, including the pre-occlusion alpha
uniform vec4 layer_state[12]; // rotation cosine/sine, texture slot, visibility and blend flags
uniform int layer_count;
uniform int disc_layer = -1;
uniform vec4 bounds_uv;
uniform vec2 sun_uv;
uniform float source_scale;
uniform bool sample_sky;
varying flat float visibility;

bool sky_at(vec2 uv) {
    return textureLod(depth_texture, uv, 0.0).r == 0.0;
}

void vertex() {
    visibility = 1.0;
    if (sample_sky) {
        visibility = 0.0;
        vec2 source_pixel = source_scale / VIEWPORT_SIZE;
        vec2 origin = trunc(sun_uv / source_pixel - vec2(16.0));
        for (int y = 0; y < 32; ++y) {
            for (int x = 0; x < 32; ++x) {
                vec2 uv = (origin + vec2(float(x), float(y)) + 0.5) * source_pixel;
                if (all(greaterThanEqual(uv, vec2(0.0))) && all(lessThan(uv, vec2(1.0)))) {
                    visibility += float(sky_at(uv));
                }
            }
        }
        visibility /= 1024.0;
    }
    // The submitted Godot screen coordinates and SCREEN_UV both start at the top left.
    POSITION = vec4(mix(bounds_uv.xy, bounds_uv.zw, UV) * 2.0 - 1.0, 0.0, 1.0);
}

// ePoly sends Q=1. TEX1 has K=0, so the GS selects MMAG at level zero.
vec4 flare_texel(int slot, vec2 uv) {
    if (slot == 0) return textureLod(flare_0, uv, 0.0);
    if (slot == 1) return textureLod(flare_1, uv, 0.0);
    if (slot == 2) return textureLod(flare_2, uv, 0.0);
    if (slot == 3) return textureLod(flare_3, uv, 0.0);
    return textureLod(flare_4, uv, 0.0);
}

vec3 to_gs_bytes(vec3 linear_rgb) {
    vec3 rgb = clamp(linear_rgb, vec3(0.0), vec3(1.0));
    return round(mix(1.055 * pow(rgb, vec3(1.0 / 2.4)) - 0.055, rgb * 12.92,
        lessThanEqual(rgb, vec3(0.0031308))) * 255.0);
}

vec3 from_gs_bytes(vec3 bytes) {
    vec3 rgb = bytes / 255.0;
    return mix(pow((rgb + 0.055) / 1.055, vec3(2.4)), rgb / 12.92,
        lessThanEqual(rgb, vec3(0.04045)));
}

void fragment() {
    vec3 destination = vec3(0.0);
    bool has_destination = false;
    // All sun textures share priority 7. The source alpha sorter walks
    // equal positive Z keys in reverse submission order, with the disc last.
    for (int order = 0; order < layer_count; ++order) {
        int i = layer_count - 1 - order;
        if (disc_layer >= 0 && i <= disc_layer) {
            i = i == 0 ? disc_layer : i - 1;
        }
        vec4 rect = layer_rect[i];
        vec4 state = layer_state[i];
        vec2 p = (SCREEN_UV - rect.xy) / rect.zw;
        vec2 uv = mat2(vec2(state.x, -state.y), vec2(state.y, state.x)) * p * 0.5 + 0.5;
        if (any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0)))) continue;
        int flags = int(state.w);
        if ((flags & 8) != 0 && !sky_at(SCREEN_UV)) continue;

        vec4 tint = layer_color[i];
        if ((flags & 16) != 0) tint.a *= visibility;
        tint.a = float(int(tint.a) & 255);

        vec4 texel = floor(flare_texel(int(state.z), uv) * 255.0 + 0.001);
        // TEX0 TFX=MODULATE, TCC=RGBA. Context TEST accepts alpha >= 0.
        vec4 source = min(floor(texel * tint / 128.0), vec4(255.0));
        if (source.a == 0.0) continue;

        if (!has_destination) {
            // The original sky meshes are depth-free and draw outside the
            // opaque scene copy. Read their color only where depth is clear.
            destination = has_sky && sky_at(SCREEN_UV)
                ? round(textureLod(sky_color, SCREEN_UV, 0.0).rgb * 255.0)
                : to_gs_bytes(textureLod(scene_color, SCREEN_UV, 0.0).rgb);
            has_destination = true;
        }
        // GS ALPHA A=Cs, C=As, D=Cd. B=Cd for blend and zero for add.
        vec3 b = (flags & 4) != 0 ? vec3(0.0) : destination;
        destination = clamp(destination + floor((source.rgb - b) * source.a / 128.0),
            vec3(0.0), vec3(255.0));
    }
    if (!has_destination) discard;
    ALBEDO = from_gs_bytes(destination);
    ALPHA = 1.0;
}
