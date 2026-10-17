#include "Sub_00152070.hpp"
#include "Sub_0018F808.hpp"
#include <cmath>
#include <stdexcept>
namespace port::driving {
// E-0038-C01: selected surface1524E8..FC publishes material byte and plane XYZW.
// This provider supplies infinite synthetic geometry directly; no original broadphase claimed.
float PlaneContact::Sub_00152070(Memory& m,std::uint32_t,std::uint32_t point,
    std::uint32_t material,std::uint32_t plane,std::uint32_t found) {
    const auto p=m.vector(point);
    for(float v:p)if(!std::isfinite(v))throw std::invalid_argument("Nonfinite synthetic plane query");
    if(material)m.word(material,1);
    if(plane)m.vector(plane,{{0,0,1,height}});
    if(found)m.word(found,1);
    return height;
}
// ELF00153458..00153497 E-0038-C02. Preserve modular low32 arithmetic.
float Sub_00153458(Memory& m,std::int32_t x,std::int32_t y) {
    const auto a=static_cast<std::uint32_t>(x),b=static_cast<std::uint32_t>(y);
    const auto index=(a*b*11u+(a+1u)*(b+1u)*13u)&15u;
    return m.scalar(0x002f94c8+index*4);
}
// ELF00153498 E-0038-C02. Floor is explicitly CVT.W.S followed by strict correction.
float Sub_00153498(Memory& m,std::uint32_t point,std::int32_t material) {
    if(material<0||material>=32)throw std::out_of_range("Material outside bounded road projection");
    const auto row=0x00318c00u+static_cast<std::uint32_t>(material)*0x90u;
    float x=m.scalar(point)*m.scalar(row+0x18),y=m.scalar(point+4)*m.scalar(row+0x18);
    float fx=static_cast<float>(cvt_w_s(x)),fy=static_cast<float>(cvt_w_s(y));
    if(x<fx)fx=fx-1.0f;
    if(y<fy)fy=fy-1.0f;
    const auto ix=cvt_w_s(fx),iy=cvt_w_s(fy);
    y=y-fy;x=x-fx;const float inverse=1.0f-x;
    float value=inverse*(1.0f-y);value=value*Sub_00153458(m,ix,iy);
    float term=x*(1.0f-y);term=term*Sub_00153458(m,ix+1,iy);value=value+term;
    term=inverse*y;term=term*Sub_00153458(m,ix,iy+1);value=value+term;
    term=x*y;term=term*Sub_00153458(m,ix+1,iy+1);value=value+term;
    return value*m.scalar(row+0x1c);
}
}
