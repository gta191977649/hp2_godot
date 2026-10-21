#pragma once
#include "game_core/port/track/texture_state.hpp"
#include "port/vehicle/base_material.hpp"
#include <numeric>
#include <optional>
#include <vector>

namespace port::track {
// CreateViewMatricies Retail0010E638, Debug0010EA18. ZRanges table bytes
// Retail002C9E20 equal Debug002C14E0. These are original screen-depth float
// values, NOT normalized GPU depth and NOT a world-space displacement.
inline std::array<float,2> screen_depth_range(std::uint8_t bias_level,bool z24=false) {
    constexpr std::uint32_t words[2][6]={{0x40000010,0x400ff000,0x40000040,0x400ffff0,0x40000ff0,0x400ffff0},
        {0x40000010,0x40800000,0x40000040,0x40802000,0x40900000,0x40f00000}};
    const unsigned pack=bias_level==1?1:bias_level==2?2:0;
    std::array<float,2> result{};
    for(unsigned i=0;i<2;++i)std::memcpy(&result[i],&words[z24?1:0][pack*2+i],4);
    return result;
}
inline std::array<float,2> screen_depth_coefficients(float near_z,float far_z,
    std::uint8_t bias_level,bool z24=false) {
    if(!(near_z>0&&far_z>near_z))throw std::invalid_argument("Invalid original depth projection");
    const auto range=screen_depth_range(bias_level,z24);
    const float a=(-range[1]*near_z+range[0]*far_z)/(far_z-near_z);
    const float b=(far_z*near_z*(range[1]-range[0]))/(far_z-near_z);
    return {{a,b}};
}
inline float screen_depth(float view_z,float near_z,float far_z,
    std::uint8_t bias_level,bool z24=false) {
    if(view_z==0)throw std::invalid_argument("Invalid original depth projection");
    const auto coefficients=screen_depth_coefficients(near_z,far_z,bias_level,z24);
    return coefficients[0]+coefficients[1]/view_z;
}
// VU stores float z bits in the PACKED XYZF2 z word (GIFtag0x412).
// XYZF2 extracts bits4..27, before the selected ZBUF storage format.
inline std::uint32_t screen_depth_to_gs(float screen_z,std::uint8_t zbuffer_mode=0x30) {
    std::uint32_t bits;std::memcpy(&bits,&screen_z,4);
    if(zbuffer_mode==0x30||zbuffer_mode==0x31)return (bits>>4)&0xffffffu;
    hp2::not_implemented("0x002A7860","Track GS depth format outside original normal targets");
}
// Port GPU projection, not original arithmetic. Double retains the complete
// integer ratio; a float32 depth attachment may lose low bits. An integer depth
// attachment plus explicit GEQUAL is required for complete GS depth precision.
inline double normalized_gpu_depth(std::uint32_t z,std::uint8_t zbuffer_mode=0x30) {
    if(zbuffer_mode==0x30)return double(z)/4294967295.0;
    if(zbuffer_mode==0x31)return double(z&0xffffffu)/16777215.0;
    hp2::not_implemented("0x002A7860","Track GPU depth format outside original normal targets");
}
// Port projection onto an existing perspective host depth buffer. Invert pack0
// rather than move geometry: identical pack0 depth stays aligned with cars.
inline double effective_inverse_view_z(float screen_z,float original_near,float original_far,bool z24=false) {
    const auto range=screen_depth_range(0,z24);
    const float a=(-range[1]*original_near+range[0]*original_far)/(original_far-original_near);
    const float b=(original_far*original_near*(range[1]-range[0]))/(original_far-original_near);
    if(!(original_far>original_near&&original_near>0&&b!=0))throw std::invalid_argument("Invalid original depth inversion");
    return (double(screen_z)-a)/b;
}
inline double host_depth_from_screen(float screen_z,float original_near,float original_far,
    double host_projection_a,double host_projection_b,bool z24=false) {
    return host_projection_a+host_projection_b*effective_inverse_view_z(screen_z,original_near,original_far,z24);
}
inline double standard_host_depth(float view_z,float near_z,float far_z,std::uint8_t bias,
    bool reversed=true,bool z24=false) {
    if(view_z==0)throw std::invalid_argument("Invalid host depth projection");
    const auto selected=screen_depth_coefficients(near_z,far_z,bias,z24);
    const auto normal=screen_depth_coefficients(near_z,far_z,0,z24);
    const double q=1.0/view_z+((double(selected[0])-normal[0])+
        (double(selected[1])-normal[1])/view_z)/normal[1];
    const double result=(-double(near_z)+double(near_z)*far_z*q)/(far_z-near_z);
    return reversed?result:1.0-result;
}
// Bounded scalar projection of original Retail VU PC0C3..138 (002A6F78).
// The caller supplies original local-space reflected vectors and uploaded blocks.
inline std::uint8_t alpha_002A6F78(const std::array<float,3>& reflected,
    const vehicle::VuMaterialBlock& material,const vehicle::VuLightBlock& lights) {
    float dot=lights[4][0]*reflected[0];
    dot+=lights[5][0]*reflected[1];dot+=lights[6][0]*reflected[2];
    const float power=vehicle::base_material_detail::affine_bit_power(
        std::max(dot,material[0][2]),material[3][0],material[3][1]);
    const float alpha=material[3][2]+material[3][3]*power;
    return static_cast<std::uint8_t>(vehicle::base_material_detail::ftoi0(std::clamp(alpha,0.f,255.f)));
}
struct SecondPass320 {
    // Retail0010AC50: GS context1 state, no texture-header blend substitution.
    std::uint64_t tex0=0x660013300ULL;
    std::uint64_t tex1=0x60,clamp=0,miptbp1=0,miptbp2=0;
    std::uint64_t alpha=0x4000000058ULL; // (Cs-0)*Ad/128+Cd; FIX64 unused.
};
struct EffectState {
    std::uint16_t flags{};
    std::uint8_t matrix_pack{};
    std::int8_t rendering_order{};
    bool alpha_pass{},rewrite_alpha{},second_pass_320{},environment_pass{};
    TextureState texture;
    vehicle::TestState test;
};
enum class EffectPassKind { Base, Specular320, Environment318, Environment31C };
struct EffectPass {
    EffectPassKind kind=EffectPassKind::Base;
    unsigned gs_context=0;
    std::uint64_t alpha_register=0;
    std::uint32_t texture_hash=0;
    bool state_available=true;
};
inline std::vector<EffectPass> pass_sequence(const EffectState& state,std::uint32_t base_texture,
    std::optional<std::uint64_t> environment_alpha=std::nullopt) {
    std::vector<EffectPass> passes{{EffectPassKind::Base,0,state.texture.alpha_register,base_texture,true}};
    if(state.second_pass_320)passes.push_back({EffectPassKind::Specular320,0,0x4000000058ULL,0x0184d1f8,true});
    if(state.environment_pass)passes.push_back({(state.flags&4)?EffectPassKind::Environment31C:EffectPassKind::Environment318,
        1,environment_alpha.value_or(0),0,environment_alpha.has_value()});
    return passes;
}
inline EffectState effect_state(const hp2::assets::TpkTextureHeader& header,
    std::uint16_t effective_flags,bool depth_route_enabled=true) {
    EffectState result;
    result.flags=effective_flags;result.rendering_order=header.RenderingOrder;
    result.matrix_pack=header.BiasLevel==1?1:header.BiasLevel==2?2:0;
    result.alpha_pass=header.Field_0x4d!=0;
    if(!result.alpha_pass&&(header.RenderingOrder<0||header.RenderingOrder>7))
        throw std::runtime_error("Original solid texture rendering-order bucket outside0..7");
    result.rewrite_alpha=result.second_pass_320=(effective_flags&0x40)!=0;
    result.environment_pass=(effective_flags&0x100)!=0;
    result.texture=texture_state_00108CC0(header);
    result.test=vehicle::test_state_00105AC0(header,false,depth_route_enabled);
    return result;
}
// Original RadixSortAlphaKeys001051D8 + reversed-list SortAlphaKeys001055B0.
// Input order must be the original linked-list traversal order. Reversal makes
// identical nonnegative depth keys reverse their input order (not stable_sort).
struct AlphaKey {std::int8_t rendering_order{};float depth{};};
inline std::vector<std::size_t> alpha_order_001055B0(const std::vector<AlphaKey>& keys) {
    std::vector<std::size_t> order(keys.size());std::iota(order.begin(),order.end(),0);
    const auto bucket=[](std::int8_t n){return n<0||n>7?7:int(n);};
    std::stable_sort(order.begin(),order.end(),[&](auto a,auto b){
        const int aa=bucket(keys[a].rendering_order),bb=bucket(keys[b].rendering_order);
        if(aa!=bb)return aa>bb;
        const auto bits=[](float f){std::uint32_t u;std::memcpy(&u,&f,4);return u;};
        const auto ua=bits(keys[a].depth),ub=bits(keys[b].depth);
        const auto ka=(ua&0x80000000u)?~ua:ua^0x80000000u;
        const auto kb=(ub&0x80000000u)?~ub:ub^0x80000000u;
        if(ka!=kb)return ka<kb;
        return (ua&0x80000000u)!=0?a>b:false;
    });
    std::reverse(order.begin(),order.end());return order;
}
// Raw GS framebuffer channel reference. Preserve destination alpha separately
// when a host normalized blend target cannot represent source alpha above128.
inline float blend_channel(float source,float destination,float source_alpha,
    float destination_alpha,std::uint8_t equation) {
    if(equation==0x0a)return source;
    if(equation==0x44)return (source-destination)*source_alpha/128.f+destination;
    if(equation==0x48)return source*source_alpha/128.f+destination;
    if(equation==0x58)return source*destination_alpha/128.f+destination;
    hp2::not_implemented("0x0010AC50","Unsupported GS blend equation");
}
inline int blend_channel_gs(std::uint8_t source,std::uint8_t destination,
    std::uint8_t source_alpha,std::uint8_t destination_alpha,std::uint8_t equation) {
    if(equation==0x0a)return source;
    int product=0;
    if(equation==0x44)product=(int(source)-int(destination))*source_alpha;
    else if(equation==0x48)product=int(source)*source_alpha;
    else if(equation==0x58)product=int(source)*destination_alpha;
    else hp2::not_implemented("0x0010AC50","Unsupported GS integer blend equation");
    const int scaled=product>=0?product/128:-((-product+127)/128);
    return scaled+destination; // Apply original FRAME/COLCLAMP separately.
}
// Bounded RGB fusion: first pass cannot fail alpha, both kicks reuse identical
// coverage/depth, COLCLAMP1 and nonnegative48/58 increments. Native command
// stream still contains two passes; host must ensure no later unfused Ad read.
inline bool can_fuse_additive_40(const EffectState& state) {
    return state.rewrite_alpha&&state.second_pass_320&&!state.environment_pass&&
        (state.flags&1)!=0&&state.texture.alpha_register==0x48&&
        state.test.alpha_test==5&&state.test.alpha_reference==0;
}
inline unsigned fused_additive_40_increment(std::uint8_t base_source,
    std::uint8_t specular_source,std::uint8_t base_source_alpha) {
    return unsigned(base_source)*base_source_alpha/128u+
        unsigned(specular_source)*base_source_alpha/128u;
}
enum class Fusion40 { None, Additive, Copy };
// Bounds must cover every original sampled mip level, not only base pixels.
inline Fusion40 fusion_40(const EffectState& state,unsigned base_alpha_min,
    unsigned base_alpha_max,unsigned specular_alpha_min,unsigned specular_alpha_max) {
    if(!state.rewrite_alpha||!state.second_pass_320||state.environment_pass||(state.flags&1)==0)
        return Fusion40::None;
    const bool alpha_always=state.test.alpha_test==5&&state.test.alpha_reference==0;
    const bool alpha_identical=base_alpha_min==base_alpha_max&&specular_alpha_min==specular_alpha_max&&
        base_alpha_min==specular_alpha_min;
    if(!alpha_always&&!alpha_identical)return Fusion40::None;
    if(state.texture.alpha_register==0x48)return Fusion40::Additive;
    if(state.texture.alpha_register==0x0a)return Fusion40::Copy;
    return Fusion40::None;
}
inline unsigned fused_copy_40_source(std::uint8_t base_source,
    std::uint8_t specular_source,std::uint8_t base_source_alpha) {
    return std::min(255u,unsigned(base_source)+unsigned(specular_source)*base_source_alpha/128u);
}
}
