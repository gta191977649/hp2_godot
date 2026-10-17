// Port depth projection of original ZRanges. This changes depth, never geometry.
varying flat vec2 track_host_depth_ab;
uniform float depth_near=0.1;
uniform float depth_far=10000.0;
vec2 hp2_track_depth_coeff(vec2 range){
    float n=depth_near,f=depth_far;
    return vec2((-range.y*n+range.x*f)/(f-n),f*n*(range.y-range.x)/(f-n));
}
float hp2_track_project_host_depth(float positive_view_z,vec2 host_projection){
    vec2 source=hp2_track_depth_coeff(@TRACK_DEPTH_RANGE@);
    vec2 base=hp2_track_depth_coeff(vec2(intBitsToFloat(0x40000010),intBitsToFloat(0x40800000)));
    float q=1.0/positive_view_z;
    float effective_q=q+((source.x-base.x)+(source.y-base.y)*q)/base.y;
    return host_projection.x+host_projection.y*effective_q;
}
