#include "game_core/port/driving/projections.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace port::driving::init;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
// Independent instruction-store oracle: explicit little-endian writes, not ByteView.
template<std::size_t N>void sw(std::array<std::uint8_t,N>& image,std::size_t o,std::uint32_t v){
    image[o]=static_cast<std::uint8_t>(v);image[o+1]=static_cast<std::uint8_t>(v>>8);
    image[o+2]=static_cast<std::uint8_t>(v>>16);image[o+3]=static_cast<std::uint8_t>(v>>24);
}
}
int main(){
    // E-0038-I05: independent 001A3F24..58 instruction-store schedule.
    std::array<std::uint8_t,0x190> wheel;wheel.fill(0xa5);auto expected=wheel;
    Sub_001A3F10(wheel,0x12345678,0x23456789,0x3456789a,0x456789ab);
    sw(expected,8,0x002b5c40);sw(expected,0x18,0x456789ab);sw(expected,0xc,0x12345678);
    sw(expected,0x10,0x23456789);sw(expected,0x14,0x3456789a);sw(expected,0x110,1);
    sw(expected,0x17c,0x3f800000);sw(expected,0x180,0);sw(expected,0x178,0x3f800000);
    expected[0x156]=0;expected[0x157]=0;sw(expected,0x164,0);sw(expected,0x15c,0);
    check(wheel==expected,"001A3F10 exact stores and untouched bytes, including halfword neighbors");
    // E-0038-I06: full store-only constructor, preserving every other byte.
    std::array<std::uint8_t,0xb0> controller;controller.fill(0x5a);auto controller_expected=controller;
    Sub_001A5EA0(controller,3,0x01020304,0x05060708,0x090a0b0c,0x0d0e0f10);
    sw(controller_expected,0x9c,3);sw(controller_expected,0x94,0x01020304);
    sw(controller_expected,8,0x002b5cd0);sw(controller_expected,0x90,0x05060708);
    sw(controller_expected,0x98,0x090a0b0c);sw(controller_expected,0xc,0x0d0e0f10);sw(controller_expected,0x10,0);
    check(controller==controller_expected,"001A5EA0 exact argument/register mapping");
    const auto identity=bIdentity();
    for(std::size_t i=0;i<16;++i)check(identity[i]==(i%5==0?1.0f:0.0f),"0025AC68 quadword lane permutations");
    // Non-orthonormal upper triangular affine input. Unused fourth lanes are sentinels.
    const std::array<float,16> matrix{{2,0,0,99, 1,4,0,98, 0,2,8,97, 6,12,24,96}};
    const std::array<float,16> inverse{{0.5f,0,0,0, -0.125f,0.25f,0,0, 0.03125f,-0.0625f,0.125f,0, -2.25f,-1.5f,-3,1}};
    check(bInvertMatrix(matrix)==inverse,"0025AC98 determinant/cofactor/translation fixture");
    // Independently multiply column-major affine matrices after normalizing ignored lanes.
    auto normalized=matrix;normalized[3]=0;normalized[7]=0;normalized[11]=0;normalized[15]=1;
    const auto actual_inverse=bInvertMatrix(matrix);
    for(int row=0;row<4;++row)for(int col=0;col<4;++col){
        double sum=0;for(int k=0;k<4;++k)sum+=double(normalized[k*4+row])*actual_inverse[col*4+k];
        check(sum==(row==col?1.0:0.0),"0025AC98 independent affine multiplication oracle");
    }
    std::array<std::uint8_t,0x50> config{};ByteView cfg(config);cfg.scalar(0,4);
    for(std::size_t i=0;i<16;++i)cfg.scalar(0x10+i*4,matrix[i]);
    std::array<std::uint8_t,0x190> body;body.fill(0xcd);auto body_expected=body;
    Sub_0019C9B0(body,0x11223344,config);ByteView b(body);
    // E-0038-I04 listing's zero-store offsets, including untouched fourth vector lanes.
    for(auto offset:{0x10,0x14,0x18,0x60,0x64,0x68,0x70,0x74,0x78,0x80,0x90,0x94,0x98,0xa0,
                    0xa4,0xa8,0xf0,0xf4,0xf8,0x100,0x104,0x108,0x170,0x174,0x114})sw(body_expected,offset,0);
    sw(body_expected,8,0x002b5430);sw(body_expected,0xc,0x11223344);
    for(std::size_t i=0;i<16;++i)sw(body_expected,0x20+4*i,i%5==0?0x3f800000:0);
    sw(body_expected,0x178,0x3f800000);sw(body_expected,0x110,1);
    sw(body_expected,0x118,0x40800000);sw(body_expected,0x11c,0x3e800000);
    // Matrix values were independently checked above; preserve sign bits from the instruction arithmetic.
    for(std::size_t i=0;i<16;++i){check(b.scalar(0x120+4*i)==inverse[i],"0019CC58 matrix inverse");
        sw(body_expected,0x120+4*i,b.word(0x120+4*i));}
    check(body==body_expected,"0019C9B0 exact store coverage without zeroing unknown bytes");
    check(b.word(0x6c)==0xcdcdcdcd&&b.word(0x7c)==0xcdcdcdcd,"Untouched vector W lanes remain unknown");
    auto before=body;Calibrate(body,nullptr);check(body==before,"0019CC40 null skips all reads and writes");
    const std::array<std::uint32_t,4> lanes{{0x7fc12345,0x80000000,0x12345678,0xffffffff}};
    Calibrate(body,&lanes);for(std::size_t i=0;i<4;++i)check(b.word(0x160+4*i)==lanes[i],"0019CC40 bit-preserving XYZW copy");
    Calibrate(controller,17,{{1,2,3,4}},{{5,4,3,2}});ByteView c(controller);
    check(c.scalar(0x78)==17&&c.scalar(0x80)==-4&&c.scalar(0x84)==-2&&c.scalar(0x88)==0&&c.scalar(0x8c)==2,"001A5F88 subtracts all four lanes");
    SetState(wheel,-3,7);ByteView w(wheel);check(w.scalar(0x1c)==-3&&w.scalar(0x20)==7,"001A4158 f12/f13 stores");
    Calibrate(wheel,0x7fc12345);check(w.word(0x24)==0x7fc12345,"001A4348 copies global raw bits");
    Sub_001A5FB0(controller);check(c.word(0x14)==0&&c.word(0x18)==0,"001A5FB0 clears exactly two fields");
    std::array<std::uint8_t,20> head;head.fill(0xa5);auto head_expected=head;
    Sub_0019C170(head,0x12345678);sw(head_expected,0,0x12345678);sw(head_expected,4,0x12345678);
    sw(head_expected,8,0);sw(head_expected,12,0x002b53a0);check(head==head_expected,"0019C170 encoded self pointers and table");
    std::array<std::uint8_t,8> truncated{};bool rejected=false;
    try{Sub_001A3F10(truncated,0,0,0,0);}catch(const std::out_of_range&){rejected=true;}
    check(rejected&&std::all_of(truncated.begin(),truncated.end(),[](auto v){return v==0;}),"Port bounds reject before writes");
    std::cout<<"Driving initialization dependency tests passed; PS2 equivalence remains unverified.\n";
}
