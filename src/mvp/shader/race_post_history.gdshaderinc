#include "race_post_shared.gdshaderinc"

uniform sampler2D focus_image : filter_nearest, repeat_disable;
uniform sampler2D previous_image : filter_linear, repeat_disable;
uniform sampler2D radial_mask : filter_linear, repeat_disable;
uniform sampler2D filter_image : filter_linear, repeat_disable;

void fragment() {
    ivec2 size = ivec2(round(1.0 / SCREEN_PIXEL_SIZE));
    ivec2 xy = ivec2(FRAGCOORD.xy);
    vec2 uv = (vec2(xy) + 0.5) / vec2(size);
#ifdef HISTORY_FIXTURE
    vec4 c = vec4(0.0, 1.0, 0.0, 1.0);
#else
    // Focus, filter and history already operate in encoded color. Monochrome
    // selects the green channel, which commutes with the transfer function.
    vec4 c = vec4(fogged(xy, use_fog_palette), 1.0);
    if (kernel.y > 0.0) {
        float slot = depth_slot(scene_depth_at(xy));
        float near_slot = clamp(floor(slot_at(focus.x-focus.y*0.5)),0.0,255.0);
        float far_slot = clamp(floor(slot_at(focus.x+focus.y*0.5)),0.0,255.0);
        float edge = max(max(slot-near_slot, far_slot-slot), 0.0);
        float alpha = mix(focus.z, focus.w, clamp(edge/max(kernel.x,1.0),0.0,1.0));
        ivec2 limit = textureSize(focus_image, 0)-1;
        ivec2 base = ivec2(floor(vec2(xy)*0.5-0.25));
        vec2 f = fract(vec2(xy)*0.5-0.25);
        vec3 a = mix(texelFetch(focus_image,clamp(base,ivec2(0),limit),0).rgb,
            texelFetch(focus_image,clamp(base+ivec2(1,0),ivec2(0),limit),0).rgb,f.x);
        vec3 b = mix(texelFetch(focus_image,clamp(base+ivec2(0,1),ivec2(0),limit),0).rgb,
            texelFetch(focus_image,clamp(base+ivec2(1),ivec2(0),limit),0).rgb,f.x);
        vec3 focused = mix(use_fog_palette ? c.rgb : to_srgb(c.rgb), mix(a,b,f.y), alpha);
        c.rgb = use_fog_palette ? focused : to_linear(focused);
    }
    if (effects.w > 0.0) c.rgb = vec3(c.g);
    if (effects.z > 0.0) {
        vec3 filter_rgb = texture(filter_image, uv).rgb * filter_color.rgb;
        vec3 dst = use_fog_palette ? c.rgb : to_srgb(c.rgb);
        vec3 filtered = clamp(dst + (dst-filter_rgb)*effects.z, 0.0, 1.0);
        c.rgb = use_fog_palette ? filtered : to_linear(filtered);
    }
#endif
#ifdef HISTORY_FIXTURE
    vec3 color = to_srgb(c.rgb);
#else
    vec3 color = use_fog_palette ? c.rgb : to_srgb(c.rgb);
#endif
    vec3 encoded = round(clamp(color, 0.0, 1.0) * 255.0);
    if (camera.z > 0.0) {
        // Retail 0010F280, Debug 0010F660 and ePoly strip 0,1,3,2.
        vec2 q = 1.0 - abs(2.0 * uv - 1.0);
        float vertex_alpha = floor(mix(effects.y, effects.x, max(0.0, q.x + q.y - 1.0)));
        float texture_alpha = history.w >= 7.0 ? 63.0 :
            floor(textureLod(radial_mask, q, 0.0).a * 255.0 + 0.0001);
        float ad = floor(texture_alpha * vertex_alpha / 128.0);
        if (ad > 0.0) {
            // Retail 0010F4F0: half a PS2 pixel at each edge of the
            // active raster. The 512-line backing padding is absent here.
            vec2 half_texel = 0.5 / vec2(512.0, history.y);
            vec2 previous_uv = mix(half_texel, 1.0 - half_texel, uv);
            vec3 previous = floor(textureLod(previous_image, previous_uv, 0.0).rgb * 255.0 + 0.001);
            float weight = ad / 128.0;
            // Keep the GS coefficient at the original cadence. Adjust only
            // fractional decay coefficients for the host render interval.
            if (weight < 1.0 && history.z != 1.0) weight = pow(weight, history.z);
            encoded = clamp(encoded + floor((previous - encoded) * weight), 0.0, 255.0);
        }
    }
    COLOR = vec4(history.x > 0.0 ? encoded / 255.0 : color, 1.0);
}
