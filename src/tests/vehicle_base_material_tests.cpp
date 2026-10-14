#include "port/vehicle/base_material.hpp"
#include <iostream>
#include <stdexcept>
namespace {
void check(bool value){if(!value)throw std::runtime_error("Vehicle base material assertion failed");}
}
int main(){
    using namespace port::vehicle;
    VuMaterialBlock material{};VuLightBlock lights{};
    // Zero power slope, float-bit intercept1.0 makes all specular weights1.
    material[0]={{0.f,float(0x3f800000),.1f,0.f}};
    material[1]={{1,1,1,100}};material[2]={{.5f,.5f,.5f,200}};
    material[3]={{0,float(0x3f800000),32,16}};
    lights[0]={{64,32,16,8}};lights[1]={{0,0,0,0}};lights[2]={{0,0,0,0}};
    lights[3]={{4,8,12,0}};lights[4]={{1,0,0,0}};
    const auto opaque=base_rgba_002A8308({{1,0,0}},{{1,0,0}},0x7fff,material,lights,false);
    check(opaque==std::array<std::uint8_t,4>{{100,56,36,8}});
    const auto flagged=base_rgba_002A8308({{1,0,0}},{{1,0,0}},0x7fff,material,lights,true);
    check(flagged==std::array<std::uint8_t,4>{{100,56,36,48}});
    material[0][3]=1.f/255.f;
    const auto red=base_rgba_002A8308({{1,0,0}},{{1,0,0}},0x001f,material,lights,true);
    check(red[0]==97 && red[1]==0 && red[2]==0);
    material[0][3]=0;lights[3]={{300,-2,12,0}};
    const auto clamped=base_rgba_002A8308({{0,1,0}},{{1,0,0}},0,material,lights,false);
    check(clamped[0]==255 && clamped[1]==14 && clamped[2]==20);
    // Original Default/Window911 normal descriptor: alpha endpoints 20/128,
    // bit-power scale8 and amplitude1.6. Endpoints do not bound final alpha.
    VuMaterialBlock window_material{};
    window_material[0][2]=.1f;
    window_material[3]={{8.f,float(base_material_detail::float_bits(1.6f))-8.f*1065353216.f,20.f,108.f}};
    VuLightBlock window_lights{};window_lights[4][0]=1.f;
    const auto window_alpha=base_rgba_002A8308({{1,0,0}},{{1,0,0}},0,window_material,window_lights,true);
    check(window_alpha[3]==192); // With raw texture alpha127, GS As=190 (>unity128).
    const auto coefficients=reflection_coefficients_0010AC50();
    const auto reflected=reflection_002A7F90({{0,0,0}},{{0,0,1}},{{0,0,1}},coefficients);
    check(reflected[0]==0 && reflected[1]==0);
    check(base_material_detail::float_bits(reflected[2])==0x3f70c480);
    // E-0033-G05 independent original-instruction oracle fixture. The private
    // scalar VU332 interpreter executes the retained original opcodes, not this
    // helper. Four lanes exercise matrix directions, bit-power and RGB factors.
    VuMaterialBlock oracle_material{{{{0,float(0x3f800000),.1f,1.f/255}},
        {{.7f,.5f,.3f,99}},{{.2f,.3f,.4f,66}},{{2,-float(0x3f800000),32,16}}}};
    VuLightBlock oracle_lights{{{{64,32,16,128}},{{32,64,96,128}},{{16,32,64,128}},
        {{2,3,4,0}},{{1,0,0,0}},{{0,1,0,0}},{{0,0,1,0}}}};
    const std::array<std::array<float,3>,4> normals{{{{1,0,0}},{{0,1,0}},{{0,0,1}},{{.5f,.5f,.5f}}}};
    const std::array<std::array<float,3>,4> rays{{{{.5f,.4f,.3f}},{{.2f,.6f,.1f}},{{.1f,.1f,.8f}},{{.5f,.5f,.5f}}}};
    const std::array<std::uint16_t,4> colors{{std::uint16_t((8<<10)|(16<<5)|31),std::uint16_t((16<<10)|(8<<5)|4),0x7fff,0}};
    const std::array<std::array<std::uint8_t,4>,4> expected_flag0{{{{67,28,19,128}},{{5,18,51,128}},{{34,55,91,128}},{{0,0,0,192}}}};
    const std::array<std::array<std::uint8_t,4>,4> expected_flag4{{{{49,12,4,36}},{{3,10,23,32}},{{14,24,37,32}},{{0,0,0,36}}}};
    for(unsigned i=0;i<4;++i){
        check(base_rgba_002A8308(normals[i],rays[i],colors[i],oracle_material,oracle_lights,false)==expected_flag0[i]);
        check(base_rgba_002A8308(normals[i],rays[i],colors[i],oracle_material,oracle_lights,true)==expected_flag4[i]);
    }
    const std::array<std::array<float,3>,4> positions{{{{0,0,0}},{{1,2,3}},{{-2,4,1}},{{.5f,-2,3}}}};
    std::array<std::array<float,3>,4> packed_normals{{{{0,0,127}},{{127,0,0}},{{0,127,0}},{{73,-73,73}}}};
    const std::array<std::array<std::uint32_t,3>,4> expected_reflection{{
        {{0,0,0x3f7ff5a0}},{{0xbea8c480,0x3f28c480,0xbf28c480}},
        {{0xbea8c480,0xbf28c480,0xbf28c480}},{{0x3d6d2889,0xbf11ea68,0xbf4baa97}}}};
    for(unsigned i=0;i<4;++i){
        for(auto& component:packed_normals[i])component*=base_material_detail::bit_float(0x3c010204);
        const auto ray=reflection_002A7F90(positions[i],{{0,0,5}},packed_normals[i],coefficients);
        for(unsigned c=0;c<3;++c)check(std::uint32_t(base_material_detail::float_bits(ray[c]))==expected_reflection[i][c]);
    }
    // E-0035-R03: independent interpreter of original VU42B..47B instructions.
    VuMaterialBlock env_material{};env_material[1][3]=64;env_material[2][3]=-16;
    const std::array<float,16> env_identity{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};
    const std::array<std::array<float,3>,4> env_rays{{{{0,0,1}},{{1,0,0}},{{0,1,0}},{{.5f,-.5f,.70710677f}}}};
    const std::array<std::array<std::uint32_t,2>,4> env_bits{{{{0x3f000000,0x3f000000}},{{0x3e1e7700,0x3f000000}},{{0x3f000000,0x3f586240}},{{0x3e346d00,0x3e346d00}}}};
    const float env_alpha[]={80,64,64,75};
    for(int i=0;i<4;++i){const auto output=environment_002A8AD8(env_rays[i],env_identity,env_material);check(output[2]==env_alpha[i]);for(int j=0;j<2;++j)check(std::uint32_t(base_material_detail::float_bits(output[j]))==env_bits[i][j]);}
    std::cout<<"VEHICLE_BASE_MATERIAL_OK\n";
}
