// Original TU src/ecstasy/eenveffectse.cpp (Debug ECOFF E-0042-S001).
// Retail authority 7335b8ed...acc2; Debug names 815b60c7...332f1b.
#include "eenveffectse.hpp"
#include "port/vehicle/base_material.hpp"
#include <cmath>
#include <cstring>
#include <stdexcept>
#include "game_core/port/not_implemented.hpp"
namespace hp2::recovered {
namespace {
std::uint32_t bits(float x){std::uint32_t b;std::memcpy(&b,&x,4);return b;}
float value(std::uint32_t b){float f;std::memcpy(&f,&b,4);return f;}
int integer(float x){return port::vehicle::base_material_detail::ftoi0(x);}
std::uint32_t alpha(float x){return std::uint32_t(integer(x))<<24;}
}
// Retail001189F8/Debug00118DC0, E-0052-P01. Eight blocks, swap8..15 with16..23.
void RemapLinearIntoClut8(port::track::postprocess::Palette& out,const port::track::postprocess::Palette& input){
    auto copy=input;for(unsigned i=0;i<256;++i){const unsigned slot=(i&~31u)|((i&8u)<<1)|((i&16u)>>1)|(i&7u);out[slot]=copy[i];}
}
// Retail00119B58/Debug00119F20: MFC1 then SRL, a bit cast, not float conversion.
unsigned MapScreenZToPaletteIndex(float z){return(bits(z)>>12)&255;}
// Retail00119B70/Debug00119F38, original ZRanges[1][0] bits40000010.
float MapPaletteIndexToScreenZ(int i){return value(0x40000010u|((unsigned(i)&255u)<<12));}
// Retail00119B90/Debug00119F58: original normal Z24 projection inverse.
float MapScreenZToViewZ(float n,float f,float s){
    if(!(n>0&&f>n))throw std::invalid_argument("Invalid postprocess near/far");
    const float lo=value(0x40000010),hi=4.f;
    const float a=(-hi*n+lo*f)/(f-n),b=(f*n*(hi-lo))/(f-n);
    return b/(s-a);
}
// Retail00119308/Debug001196D0, E-0052-P01. MAX + integer/float reinterpret
// affine power follows EE listing; std::pow is not equivalent.
port::track::postprocess::PaletteResult CreateFogPalette(const port::track::postprocess::View& view,
    const port::track::postprocess::Controls& controls){
    using namespace port::track::postprocess;PaletteResult out;
    const auto& w=view.weather;const auto rgb=w.FogColour&0xffffffu;const float scale=w.Fog*128.f;
    if(!std::isfinite(w.FogStart)||!std::isfinite(scale)||!std::isfinite(controls.FogPower))throw std::invalid_argument("Nonfinite original fog input");
    if(controls.FogMode<0||controls.FogMode>3)throw std::invalid_argument("Invalid original FogModes");
    if(controls.FogMode==2){for(unsigned i=0;i<256;++i)out.linear[i]=0x80000000u|i|(i<<8)|(i<<16);}
    else if(controls.FogMode==3){const int i=integer(w.FogStart);if(i<0||i>255)throw std::invalid_argument("Fog single index outside256 slots");out.linear[i]=rgb|alpha(scale);}
    else for(unsigned i=0;i<256;++i){
        float t;
        if(controls.FogMode==1)t=float(i)<w.FogStart?1.f:1.f-(float(i)-w.FogStart)/(255.f-w.FogStart);
        else {t=float(i)<w.FogStart?0.f:float(i)<255?(float(i)-w.FogStart)/(255.f-w.FogStart):1.f;
            const float bias=1065353216.f-controls.FogPower*1065353216.f;
            t=port::vehicle::base_material_detail::affine_bit_power(std::max(t,value(0x3c23d70a)),controls.FogPower,bias);}
        out.linear[255-i]=(controls.FogMode==1?0:rgb)|alpha(t*scale);
    }
    RemapLinearIntoClut8(out.clut,out.linear);out.enabled=true;return out;
}
// Retail00119C20/Debug00119FE8, E-0052-P02. Original reversed distance slots
// and linear transition-slot writes, not a modern optical circle-of-confusion.
port::track::postprocess::PaletteResult CreateFocusPalette(const port::track::postprocess::View& view,
    const port::track::postprocess::Controls& c){
    using namespace port::track::postprocess;PaletteResult out;
    const float focal=c.FocalDistance==0?view.camera_focal_distance:c.FocalDistance;
    const float width=c.DepthOfField==0?view.camera_depth_of_field:c.DepthOfField;
    if(focal==0&&width==0)return out;
    if(c.dofNumTransitionSlots<0||c.dofNumTransitionSlots>256)throw std::invalid_argument("Original focus transition slots outside256");
    std::array<float,256> distances{};for(unsigned i=0;i<256;++i)distances[255-i]=MapScreenZToViewZ(view.near_z,view.far_z,MapPaletteIndexToScreenZ(i));
    int start=0,end=255;float prior=value(0xff7fffff);const float lower=focal-width*.5f,upper=focal+width*.5f;
    for(int i=0;i<256;++i){if(prior<lower&&lower<=distances[i])start=i;if(prior<upper&&upper<=distances[i])end=i;prior=distances[i];}
    for(int i=0;i<256;++i)out.linear[255-i]=0x808080u|alpha(i<start||i>end?c.FocusAlphaMax:c.FocusAlphaMin);
    for(int i=1;i<c.dofNumTransitionSlots;++i){const auto a=alpha(c.FocusAlphaMin+(float(i)/float(c.dofNumTransitionSlots))*(c.FocusAlphaMax-c.FocusAlphaMin));
        if(unsigned(start-i)<256)out.linear[255-(start-i)]=0x808080u|a;
        if(unsigned(end+i)<256)out.linear[255-(end+i)]=0x808080u|a;}
    RemapLinearIntoClut8(out.clut,out.linear);out.enabled=true;return out;
}
}

// TASK0052 typed packet-stage projections. Original DMA allocation/VRAM lifetime
// belongs to the adapter; these retain per-view gates, order, state and sizes.
namespace hp2::recovered {
bool eAttachDepthIntoAlpha(const port::track::postprocess::View&,const port::track::postprocess::Controls& controls,
    port::track::postprocess::Frame& frame,unsigned width,unsigned height){
    using namespace port::track::postprocess;
    if(!controls.EnableDepthIntoAlpha)return false;
    frame.passes.push_back({Stage::DepthIntoAlpha,5,{2,2,2,0,128},false,true,width,height,width,height});
    return true;
}
bool eAttachFog(const port::track::postprocess::View& view,const port::track::postprocess::Controls& controls,
    port::track::postprocess::Frame& frame,unsigned width,unsigned height){
    using namespace port::track::postprocess;
    if(!view.active||!view.target_active||(view.id!=1&&view.id!=2))return false;
    if(controls.DrawFog){frame.fog=hp2::recovered::CreateFogPalette(view,controls);
        constexpr Blend modes[4]={{0,1,0,1,128},{1,0,0,2,128},{0,1,0,1,128},{0,1,0,1,128}};
        frame.passes.push_back({Stage::Fog,5,modes[controls.FogMode],true,true,width,height,width,height});}
    return controls.DrawFog;
}
bool eAttachDepthOfField(const port::track::postprocess::View& view,const port::track::postprocess::Controls& controls,
    port::track::postprocess::Frame& frame,unsigned width,unsigned height){
    using namespace port::track::postprocess;
    if(!view.active||(view.id!=1&&view.id!=2))return false;
    if(controls.DrawDepthOfField){frame.focus=hp2::recovered::CreateFocusPalette(view,controls);if(frame.focus.enabled){
        if(controls.dofNumPasses<0||controls.dofNumPasses>2)
            hp2::not_implemented("0x00119F70","Nondefault DOF jitter table extent not established");
        frame.passes.push_back({Stage::FocusMask,5,{0,0,0,1,128},false,true,width,height,width,height});
        frame.passes.push_back({Stage::FocusClear,5,{2,2,2,0,0},true,true,0,0,width/2,height/2});
        constexpr std::array<int,2> offsets[2]={{{0,0}},{{32,32}}};
        for(int i=0;i<controls.dofNumPasses;++i){Pass pass{Stage::FocusJitter,5,{0,2,2,1,std::uint8_t(controls.JitterBlend)},true,true,width,height,width/2,height/2};pass.source_offset_fixed16=offsets[i];pass.tex1=0x61;frame.passes.push_back(pass);}
        Pass composite{Stage::FocusComposite,5,{0,1,1,1,128},true,true,width/2,height/2,width,height};composite.tex1=0x61;frame.passes.push_back(composite);
    }}
    return frame.focus.enabled;
}
}

namespace hp2::recovered {
// Retail0011A990/Debug0011AD58, src/ecstasy/eenveffectse.cpp.
port::track::postprocess::PaletteResult SetupBlackAndWhite(){
    port::track::postprocess::PaletteResult result;
    for(unsigned i=0;i<256;i++)result.linear[i]=0x80000000u|i|(i<<8)|(i<<16);
    RemapLinearIntoClut8(result.clut,result.linear);result.enabled=true;return result;
}
}

namespace hp2::recovered {
std::uint32_t eAttachBlackAndWhite(std::uint32_t current,std::uint8_t main_display_alpha){
    const auto index=((current>>8)&0xfeu)|(main_display_alpha&1u);
    return 0x80000000u|index|(index<<8)|(index<<16);
}
bool eAttachBlackAndWhite(const port::track::postprocess::View& view,
    const port::track::postprocess::Controls& controls,port::track::postprocess::Frame& frame,unsigned width,unsigned height){
    using namespace port::track::postprocess;
    if((view.id!=1&&view.id!=2)||(!controls.BlackAndWhiteOverride&&(!view.active||!view.target_active||!view.BlackAndWhiteMode)))return false;
    frame.passes.push_back({Stage::BlackAndWhite,5,{2,2,2,0,128},true,true,width,height,width,height});return true;
}
}
