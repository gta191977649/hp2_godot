#include "game_core/port/driving/projections.hpp"
#include "game_core/port/driving/projections.hpp"
#include "game_core/port/driving/projections.hpp"
#include "game_core/port/driving/projections.hpp"
#include <iostream>
#include <stdexcept>
using namespace port::driving;
namespace {void check(bool v,const char* message){if(!v)throw std::runtime_error(message);} }
int main(){
    const auto identity=init::bIdentity();
    const auto derivative=native::VectorStarMatrix({{0,0,2,99}},identity);
    const std::array<float,16> expected{{0,2,0,0,-2,0,0,0,0,0,0,0,0,0,0,1}};
    check(derivative==expected,"Angular matrix derivative retains original affine last lane");
    const std::array<float,16> scale{{2,0,0,0,0,3,0,0,0,0,4,0,5,6,7,1}};
    check(native::bMulMatrix(identity,scale)==scale,"Matrix product identity left");
    auto product=native::bMulMatrix(scale,scale);
    check(product[0]==4&&product[5]==9&&product[10]==16&&product[12]==15&&product[13]==24&&product[14]==35,"Column-major affine product fixture");
    Memory m;m.map(0x300000,0x40000);m.map(0x2f9000,0x1000);m.vector(0x320a60,{{0,0,0,1}});
    const auto body=m.allocate(0x180);m.matrix(body+0x20,identity);m.matrix(body+0x120,identity);m.vector(body+0x60,{{4,8,12,7}});m.vector(body+0x70,{{3,4,5,6}});m.scalar(body+0x118,4);m.scalar(body+0x11c,0.25f);m.scalar(0x330b28,2);
    native::Sub_0019D848(m,body);
    check(m.vector(body+0x90)==native::Vector{{1,2,3,1.75f}},"Body linear velocity scale includes W");
    check(m.vector(body+0xa0)==native::Vector{{3,4,5,1}},"Angular transform consumes explicit global W");
    check(m.vector(body+0xf0)==native::Vector{{0,0,-8,0}},"Gravity term is recovered force, not host physics");
    m.matrix(body+0x20,{{2,0,0,7,0.5f,3,0,8,0,0,4,9,99,98,97,0}});native::eOrthonormalizeMatrix(m,body+0x20);
    check(m.matrix(body+0x20)==identity,"Orthonormalization clears all affine nonrotation lanes");
    const auto point=m.allocate(16),material=m.allocate(4),plane=m.allocate(16),found=m.allocate(4);
    m.vector(point,{{9,-7,20,1}});PlaneContact contact;contact.height=3;
    check(contact.GetElevation(m,0,point,material,plane,found)==3&&m.word(material)==1&&m.word(found)==1&&m.vector(plane)==native::Vector{{0,0,1,3}},"Synthetic geometry publishes audited selected-surface contract");
    // Noise hash: x=y=0 -> index13; x1,y0->index10; x0,y1->index10;
    // midpoint XY(.5,.5) averages entries13,10,10,15.
    for(unsigned i=0;i<16;++i)m.scalar(0x2f94c8+4*i,static_cast<float>(i));
    m.scalar(0x318c18+0x90,1);m.scalar(0x318c1c+0x90,2);m.vector(point,{{0.5f,0.5f,0,0}});
    check(Sub_00153458(m,0,0)==13&&Sub_00153498(m,point,1)==24,"Bilinear original noise hash fixture");
    std::cout<<"DRIVING_BODY_OK: instruction and synthetic adapter fixtures\n";
}
