#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace port::vehicle {
using VuVector = std::array<float,4>;
using VuMaterialBlock = std::array<VuVector,4>;
using VuLightBlock = std::array<VuVector,7>;
// 0010AC50 packs VU29C from original globals0032F750..75C.
// Static frontend inputs: BF000000,3F800000,3F800000,3F70C49C.
inline std::array<float,2> reflection_coefficients_0010AC50(){
    const std::uint32_t bits=0x4ebe6189;float bias;std::memcpy(&bias,&bits,4);
    return {{-.5f,bias}};
}
namespace base_material_detail {
inline float bit_float(std::int32_t bits){float value;std::memcpy(&value,&bits,4);return value;}
inline std::int32_t float_bits(float value){std::int32_t bits;std::memcpy(&bits,&value,4);return bits;}
inline std::int32_t ftoi0(float value){
    if(!std::isfinite(value)||double(value)<double(std::numeric_limits<std::int32_t>::min())||
       double(value)>double(std::numeric_limits<std::int32_t>::max()))
        throw std::runtime_error("Unsupported VU base material conversion range");
    return static_cast<std::int32_t>(value);
}
inline float affine_bit_power(float value,float scale,float bias){
    // Original ITOF0 converts the FLOAT BIT PATTERN as a signed integer;
    // the subsequent FTOI0 result becomes a float bit pattern, not its value.
    const float integer_value=static_cast<float>(float_bits(value));
    const float product=integer_value*scale;
    return bit_float(ftoi0(bias+product));
}
}
// E-0035-R03: VU PC42B..47B (002A8AD8), original second-context environment ST/alpha.
// Matrix is original part * environment view. No Fresnel or host reflection model.
inline std::array<float,3> environment_002A8AD8(const std::array<float,3>& reflected,
    const std::array<float,16>& matrix,const VuMaterialBlock& material){
    std::array<float,3> direction{};
    for(int i=0;i<3;++i)direction[i]=reflected[0]*matrix[i]+reflected[1]*matrix[4+i]+reflected[2]*matrix[8+i];
    direction[0]=-direction[0];direction[2]=-direction[2];
    const float z=direction[2]+1.f;
    float square=direction[0]*direction[0]+direction[1]*direction[1];square+=z*z;
    const auto coefficients=reflection_coefficients_0010AC50();
    const float inverse=base_material_detail::affine_bit_power(square,coefficients[0],coefficients[1]);
    const float alpha=material[1][3]+material[2][3]*direction[2];
    return {{.5f+(direction[0]*inverse)*.5f,.5f+(direction[1]*inverse)*.5f,
             float(base_material_detail::ftoi0(alpha))}};
}
// Port scalar projection of original VU PC2C4..307, ELF002A7F90..002A81B0.
// E-0033-G05. Coefficients come from uploaded vector VU29C, never host lights.
inline std::array<float,3> reflection_002A7F90(
    const std::array<float,3>& position,const std::array<float,3>& camera,
    const std::array<float,3>& normal,const std::array<float,2>& coefficients){
    std::array<float,3> delta{},result{};
    for(unsigned i=0;i<3;++i)delta[i]=position[i]-camera[i];
    float twice_dot=delta[0]*normal[0];twice_dot+=delta[0]*normal[0];
    twice_dot+=delta[1]*normal[1];twice_dot+=delta[1]*normal[1];
    twice_dot+=delta[2]*normal[2];twice_dot+=delta[2]*normal[2];
    for(unsigned i=0;i<3;++i)result[i]=delta[i]-normal[i]*twice_dot;
    float length_squared=result[0]*result[0];length_squared+=result[1]*result[1];length_squared+=result[2]*result[2];
    const float inverse=base_material_detail::affine_bit_power(length_squared,coefficients[0],coefficients[1]);
    for(auto& component:result)component*=inverse;
    return result;
}
// Port scalar projection of original VU PC332..3D9, ELF002A8308..002A8840.
// Material vectors are VU350..353; lights are VU35C..362. Packed VIF V4_5
// colors enter RGBAQ in low-five-bit-first component order. No host shading.
inline std::array<std::uint8_t,4> base_rgba_002A8308(
    const std::array<float,3>& normal,const std::array<float,3>& reflected,
    std::uint16_t packed_color,const VuMaterialBlock& material,
    const VuLightBlock& lights,bool flag_0x4){
    std::array<float,3> diffuse_dot{},specular_power{};
    for(unsigned light=0;light<3;++light){
        float diffuse=lights[4][light]*normal[0];diffuse+=lights[5][light]*normal[1];diffuse+=lights[6][light]*normal[2];
        diffuse_dot[light]=std::max(diffuse,0.f);
        float specular=lights[4][light]*reflected[0];specular+=lights[5][light]*reflected[1];specular+=lights[6][light]*reflected[2];
        specular=std::max(specular,material[0][2]);
        const auto& power_block=flag_0x4?material[3]:material[0];
        specular_power[light]=base_material_detail::affine_bit_power(specular,power_block[0],power_block[1]);
    }
    std::array<std::uint8_t,4> output{};
    for(unsigned component=0;component<4;++component){
        float value=component==3&&flag_0x4?material[3][2]:lights[3][component];
        for(unsigned light=0;light<3;++light){
            float coefficient=lights[light][component]*material[1][component];
            if(component==3)coefficient=flag_0x4?0:lights[light][3];
            value+=coefficient*diffuse_dot[light];
        }
        for(unsigned light=0;light<3;++light){
            float coefficient=lights[light][component]*material[2][component];
            if(component==3)coefficient=flag_0x4&&light==0?material[3][3]:0;
            value+=coefficient*specular_power[light];
        }
        if(component<3){
            const float packed=float(((packed_color>>(component*5))&31)*8);
            const float factor=1.f+(packed-255.f)*material[0][3];
            value*=factor;
        }
        if(!std::isfinite(value))throw std::runtime_error("Non-finite VU base material output");
        output[component]=static_cast<std::uint8_t>(std::clamp(value,0.f,255.f));
    }
    return output;
}
}
