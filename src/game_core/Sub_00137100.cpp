#include "Sub_00137100.hpp"
#include <cstring>
#include <stdexcept>

namespace port::driving::init {
void ByteView::require(std::size_t offset,std::size_t length) const {
    if(!data || offset>size || length>size-offset) throw std::out_of_range("PS2 initialization image is truncated");
}
std::uint32_t ByteView::word(std::size_t offset) const {
    require(offset,4);
    return std::uint32_t(data[offset]) | (std::uint32_t(data[offset+1])<<8) |
           (std::uint32_t(data[offset+2])<<16) | (std::uint32_t(data[offset+3])<<24);
}
void ByteView::word(std::size_t offset,std::uint32_t value) const {
    require(offset,4);
    for(unsigned i=0;i<4;++i)data[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}
float ByteView::scalar(std::size_t offset) const {
    auto bits=word(offset);float value;static_assert(sizeof(value)==sizeof(bits));
    std::memcpy(&value,&bits,4);return value;
}
void ByteView::scalar(std::size_t offset,float value) const {
    std::uint32_t bits;std::memcpy(&bits,&value,4);word(offset,bits);
}
std::array<float,16> Sub_0025AC68(){
    return {{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1}};
}
std::array<float,16> Sub_0025AC98(const std::array<float,16>& a){
    // Arithmetic order follows 0025ACC0..0025AE00; do not replace with a library inverse.
    const float c12=a[4]*a[10]-a[6]*a[8];
    const float c3=a[5]*a[10]-a[6]*a[9];
    const float c19=a[4]*a[9]-a[5]*a[8];
    const float reciprocal=1.0f/((a[0]*c3-a[1]*c12)+a[2]*c19);
    const float negative=-reciprocal;
    std::array<float,16> b;
    b[15]=1; b[3]=0; b[7]=0; b[11]=0;
    b[0]=reciprocal*c3;
    b[2]=reciprocal*(a[1]*a[6]-a[2]*a[5]);
    b[4]=negative*c12;
    b[6]=negative*(a[0]*a[6]-a[2]*a[4]);
    b[1]=negative*(a[1]*a[10]-a[2]*a[9]);
    b[5]=reciprocal*(a[0]*a[10]-a[2]*a[8]);
    b[10]=reciprocal*(a[0]*a[5]-a[1]*a[4]);
    b[8]=reciprocal*c19;
    b[9]=negative*(a[0]*a[9]-a[1]*a[8]);
    b[14]=-((a[12]*b[2]+a[13]*b[6])+a[14]*b[10]);
    b[12]=-((a[12]*b[0]+a[13]*b[4])+a[14]*b[8]);
    b[13]=-((a[12]*b[1]+a[13]*b[5])+a[14]*b[9]);
    return b;
}
void Sub_0019CC58(ByteView out,ByteView config){
    out.require(0,0x160);config.require(0,0x50);
    const float f1=config.scalar(0);
    std::array<float,16> matrix;
    for(std::size_t i=0;i<16;++i)matrix[i]=config.scalar(0x10+4*i);
    const auto inverse=Sub_0025AC98(matrix);
    out.scalar(0x118,f1);out.scalar(0x11c,1.0f/f1);
    for(std::size_t i=0;i<16;++i)out.scalar(0x120+4*i,inverse[i]);
}
void Sub_0019C9B0(ByteView out,std::uint32_t config_address,ByteView config){
    out.require(0,0x17c);config.require(0,0x50);
    out.word(0x10,0);out.word(0x14,0);out.word(0x18,0);
    out.word(8,0x002b5430);out.word(0xc,config_address);
    const auto matrix=Sub_0025AC68();
    for(std::size_t i=0;i<16;++i)out.scalar(0x20+4*i,matrix[i]);
    for(auto offset:{0x60,0x64,0x68,0x70,0x74,0x78,0x80,0x90,0x94,0x98,
                    0xa0,0xa4,0xa8,0xf0,0xf4,0xf8,0x100,0x104,0x108,0x170,0x174,0x114})out.word(offset,0);
    out.scalar(0x178,1);out.word(0x110,1);
    Sub_0019CC58(out,config);
}
void Sub_001A3F10(ByteView out,std::uint32_t a1,std::uint32_t a2,std::uint32_t a3,std::uint32_t t0){
    out.require(0,0x184);
    out.word(8,0x002b5c40);out.word(0x18,t0);out.word(0xc,a1);out.word(0x10,a2);out.word(0x14,a3);
    out.word(0x110,1);out.scalar(0x17c,1);out.word(0x180,0);out.scalar(0x178,1);
    out.data[0x156]=0;out.data[0x157]=0;out.word(0x164,0);out.word(0x15c,0);
}
void Sub_001A5EA0(ByteView out,std::uint32_t a1,std::uint32_t a2,std::uint32_t a3,std::uint32_t t0,std::uint32_t t1){
    out.require(0,0xa0);
    out.word(0x9c,a1);out.word(0x94,a2);out.word(8,0x002b5cd0);out.word(0x90,a3);
    out.word(0x98,t0);out.word(0xc,t1);out.word(0x10,0);
}
void Sub_0019C170(ByteView out,std::uint32_t address){
    out.require(0,0x10);out.word(0,address);out.word(0xc,0x002b53a0);out.word(4,address);out.word(8,0);
}
void Sub_0019CC40(ByteView out,const std::array<std::uint32_t,4>* a1){
    if(a1){out.require(0x160,16);for(std::size_t i=0;i<4;++i)out.word(0x160+4*i,(*a1)[i]);}
}
void Sub_001A5F88(ByteView out,float f12,const std::array<float,4>& a,const std::array<float,4>& b){
    out.require(0x78,0x18);out.scalar(0x78,f12);
    for(std::size_t i=0;i<4;++i)out.scalar(0x80+4*i,a[i]-b[i]);
}
void Sub_001A4348(ByteView out,std::uint32_t global){out.word(0x24,global);}
void Sub_001A4158(ByteView out,float f12,float f13){out.require(0x1c,8);out.scalar(0x20,f13);out.scalar(0x1c,f12);}
void Sub_001A5FB0(ByteView out){out.require(0x14,8);out.word(0x18,0);out.word(0x14,0);}
}
