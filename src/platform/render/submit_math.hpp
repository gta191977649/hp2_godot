#pragma once
// Port-side host arithmetic. Operation order follows pinned godot-cpp math,
// not PS2 ABI or recovered scheduling. Workers never access engine objects.
#include <array>
#include <cmath>
#include <cstring>
#include <cstddef>
namespace port::render {
using SubmitVector=std::array<float,3>;
struct SubmitTransform {
    std::array<float,9> basis{{1,0,0,0,1,0,0,0,1}}; // row major
    SubmitVector origin{};
};
inline float submit_dot(const SubmitVector& a,const SubmitVector& b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
inline SubmitVector submit_xform_basis(const SubmitTransform& t,const SubmitVector& v){return {{t.basis[0]*v[0]+t.basis[1]*v[1]+t.basis[2]*v[2],t.basis[3]*v[0]+t.basis[4]*v[1]+t.basis[5]*v[2],t.basis[6]*v[0]+t.basis[7]*v[1]+t.basis[8]*v[2]}};}
inline SubmitVector submit_xform(const SubmitTransform& t,const SubmitVector& v){auto out=submit_xform_basis(t,v);for(unsigned i=0;i<3;++i)out[i]+=t.origin[i];return out;}
inline bool same_submit_transform(const SubmitTransform& a,const SubmitTransform& b){return std::memcmp(a.basis.data(),b.basis.data(),sizeof(a.basis))==0&&std::memcmp(a.origin.data(),b.origin.data(),sizeof(a.origin))==0;}
inline bool submit_inverse(const SubmitTransform& t,SubmitTransform& out){
    const auto& b=t.basis;
    const auto cofac=[&](unsigned a,unsigned c,unsigned d,unsigned e){return b[a*3+d]*b[c*3+e]-b[a*3+e]*b[c*3+d];};
    const float co[3]{cofac(1,2,1,2),cofac(1,2,2,0),cofac(1,2,0,1)};
    const float determinant=b[0]*co[0]+b[1]*co[1]+b[2]*co[2];
    if(!std::isfinite(determinant)||determinant==0)return false;
    const float s=1.f/determinant;
    out.basis={{co[0]*s,cofac(0,2,2,1)*s,cofac(0,1,1,2)*s,
                co[1]*s,cofac(0,2,0,2)*s,cofac(0,1,2,0)*s,
                co[2]*s,cofac(0,2,1,0)*s,cofac(0,1,0,1)*s}};
    out.origin=submit_xform_basis(out,{{-t.origin[0],-t.origin[1],-t.origin[2]}});
    for(float v:out.basis)if(!std::isfinite(v))return false;
    for(float v:out.origin)if(!std::isfinite(v))return false;
    return true;
}
struct SubmitPoseCache {SubmitTransform source,inverse;bool valid=false;};
struct SubmitMathInput {
    SubmitTransform local,parent;
    SubmitPoseCache cached;
    SubmitVector camera{},direction{};
    std::array<float,28> lights{};
    std::array<float,16> model{},reflection_matrix{};
    std::size_t command_count=0,rank=0;
    bool reflection=false,needs_local_camera=true;
};
struct SubmitMathOutput {
    SubmitVector local_camera{},source_camera{};
    std::array<std::array<float,4>,7> lights{};
    std::array<float,9> reflection_rows{};
    SubmitPoseCache pose;
    float origin_depth=0,sorting_offset=0,reflection_sorting_offset=0;
    bool supported=false,inverse_reused=false;
};
inline SubmitMathOutput prepare_submit_math(const SubmitMathInput& in){
    SubmitMathOutput out;
    if(in.rank>in.command_count)return out;
    for(float v:in.local.basis)if(!std::isfinite(v))return out;
    for(float v:in.local.origin)if(!std::isfinite(v))return out;
    out.pose.source=in.local;
    if(in.needs_local_camera){
    if(in.cached.valid&&same_submit_transform(in.cached.source,in.local)){out.pose.inverse=in.cached.inverse;out.inverse_reused=true;}
    else if(!submit_inverse(in.local,out.pose.inverse))return out;
    out.pose.valid=true;
    out.local_camera=submit_xform(out.pose.inverse,in.camera);
    out.source_camera={{out.local_camera[2],out.local_camera[0],out.local_camera[1]}};
    }
    for(unsigned i=0;i<7;++i)for(unsigned k=0;k<4;++k)out.lights[i][k]=in.lights[i*4+k];
    const auto origin=submit_xform(in.parent,in.local.origin);
    const SubmitVector delta{{origin[0]-in.camera[0],origin[1]-in.camera[1],origin[2]-in.camera[2]}};
    out.origin_depth=submit_dot(delta,in.direction);
    out.sorting_offset=out.origin_depth-float(in.command_count-in.rank)*4096.f;
    out.reflection_sorting_offset=out.origin_depth-(float(in.command_count-in.rank)-.5f)*4096.f;
    if(in.reflection)for(unsigned r=0;r<3;++r)for(unsigned a=0;a<3;++a)out.reflection_rows[r*3+a]=in.model[r*4]*in.reflection_matrix[a]+in.model[r*4+1]*in.reflection_matrix[4+a]+in.model[r*4+2]*in.reflection_matrix[8+a];
    if(!std::isfinite(out.origin_depth)||!std::isfinite(out.sorting_offset)||!std::isfinite(out.reflection_sorting_offset))return out;
    for(float v:out.local_camera)if(!std::isfinite(v))return out;
    for(float v:out.reflection_rows)if(!std::isfinite(v))return out;
    out.supported=true;return out;
}
} // namespace port::render
