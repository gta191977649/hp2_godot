// TASK0052 host shader arithmetic for original GS postprocess packets.
// Colors/alpha are original byte units. No 3D color grading or optical DOF.
int hp2_post_depth_alpha(float original_screen_z) {
    uint z=(floatBitsToUint(original_screen_z)>>4u)&0xffffffu;
    return int(min(255u,z>>8u));
}
int hp2_post_shift7(int product) {
    return product>=0?product/128:-((-product+127)/128);
}
ivec4 hp2_post_blend(ivec4 source,ivec4 destination,ivec4 abcd,int fix) {
    ivec3 a=abcd.x==0?source.rgb:abcd.x==1?destination.rgb:ivec3(0);
    ivec3 b=abcd.y==0?source.rgb:abcd.y==1?destination.rgb:ivec3(0);
    ivec3 d=abcd.w==0?source.rgb:abcd.w==1?destination.rgb:ivec3(0);
    int c=abcd.z==0?source.a:abcd.z==1?destination.a:fix;
    ivec3 product=(a-b)*c;
    return ivec4(clamp(d+ivec3(hp2_post_shift7(product.r),hp2_post_shift7(product.g),
        hp2_post_shift7(product.b)),ivec3(0),ivec3(255)),source.a);
}
// The CPU supplies the exact original256-entry linear LUT. Use nearest sampling;
// CLUT8 physical permutation is native upload storage, not a second color LUT.
ivec4 hp2_post_palette(sampler2D palette_texture,int original_index) {
    return ivec4(round(texelFetch(palette_texture,ivec2(original_index&255,0),0)*255.0));
}
// Original two-sample downscaled focus accumulation. Explicit source-pixel offset
// is JitterOffsets fixed16 /16; the output target itself is half native size.
vec2 hp2_post_jitter_uv(vec2 uv,vec2 original_source_size,ivec2 offset_fixed16) {
    return uv+vec2(offset_fixed16)/(16.0*original_source_size);
}
