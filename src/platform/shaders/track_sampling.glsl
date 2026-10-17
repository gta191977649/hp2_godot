// Track host sampling adapter. Only original supplied mip images are sampled.
@MIP_DECLARATIONS@
vec4 sample_original(int level,vec2 uv,bool linear){
@SAMPLE_CASES@
    return vec4(0.0);
}
vec4 sample_track(vec2 uv){
    vec2 size=vec2(textureSize(base_texture,0));
    vec2 dx=dFdx(uv*size),dy=dFdy(uv*size);
    float rho=max(dot(dx,dx),dot(dy,dy));
    float lod=0.5*log2(max(rho,0.000001));
    lod=lod*@LOD_SCALE@+@LOD_BIAS@;
    if(lod<=0.0)return sample_original(0,uv,true);
@MINIFY_SOURCE@
}
