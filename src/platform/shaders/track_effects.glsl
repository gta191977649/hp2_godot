// TASK0051 Retail VU PC0C3..138, ELF002A6F78. Requires original affine-bit
// power helper (ITOF0/FTOI0 bit reinterpretation), not GLSL pow().
uint hp2_track_gs_depth(float screen_z,int zbuffer_mode) {
    // Normal geometry GIFtag0x412 writes PACKED XYZF2, z bits4..27.
    return (floatBitsToUint(screen_z)>>4u)&0xffffffu;
}
// CPU supplies original selected and pack0 (A,B), plus actual host projection
// (A,B), all in positive camera-forward coordinates. Pack0 keeps cars aligned.
float hp2_track_host_depth(float view_z,vec2 original_pack,vec2 original_pack0,
                          vec2 host_projection) {
    // Algebraic delta avoids cancellation of two screen-z values near2.
    // This is the stable host projection, separate from raw GS z-bit output.
    float inverse_z=1.0/view_z+((original_pack.x-original_pack0.x)+
        (original_pack.y-original_pack0.y)/view_z)/original_pack0.y;
    return host_projection.x+host_projection.y*inverse_z;
}
int hp2_track_blend_channel(int source,int destination,int source_alpha,
                           int destination_alpha,int equation) {
    if(equation==10)return source;
    int product=equation==68?(source-destination)*source_alpha:
        source*(equation==88?destination_alpha:source_alpha);
    int scaled=product>=0?product/128:-((-product+127)/128);
    return scaled+destination; // GS COLCLAMP applied by framebuffer stage.
}
ivec3 hp2_track_fused40_increment(ivec3 base_source,ivec3 specular_source,int base_alpha) {
    return (base_source*base_alpha)/128+(specular_source*base_alpha)/128;
}
ivec3 hp2_track_fused40_copy(ivec3 base_source,ivec3 specular_source,int base_alpha) {
    return min(ivec3(255),base_source+(specular_source*base_alpha)/128);
}
float hp2_track_alpha40(vec3 reflected,vec4 material0,vec4 material3,
                       vec3 light_direction0) {
    float d=light_direction0.x*reflected.x;
    d+=light_direction0.y*reflected.y;d+=light_direction0.z*reflected.z;
    float p=hp2_affine_bit_power(max(d,material0.z),material3.x,material3.y);
    return trunc(clamp(material3.z+material3.w*p,0.0,255.0));
}
// Raw GS channel units. Destination must be sampled explicitly for alpha>128
// and for original second-pass ALPHA58 (destination alpha, NOT source alpha).
vec3 hp2_track_blend(vec3 source,vec3 destination,float source_alpha,
                     float destination_alpha,int equation) {
    if(equation==10)return source;
    if(equation==68)return (source-destination)*(source_alpha/128.0)+destination;
    if(equation==72)return source*(source_alpha/128.0)+destination;
    return source*(destination_alpha/128.0)+destination; // bounded ALPHA58
}
