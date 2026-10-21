#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace port::track::postprocess {
// Port-owned carriers of original globals/view/weather fields; no PS2 ABI.
using Palette=std::array<std::uint32_t,256>;
struct Weather {std::uint32_t FogColour=0;float FogStart=0,Fog=0;};
struct Controls {
    int FogMode=0;
    float FogPower=4,FocusAlphaMin=0,FocusAlphaMax=128,FocalDistance=0,DepthOfField=0;
    int dofNumTransitionSlots=24,dofNumPasses=2,dofOffsetX=0x4008,dofOffsetY=0x4008;
    bool EnableDepthIntoAlpha=true,DrawFog=true,DrawDepthOfField=true;
    int JitterBlend=64;
    int BlackAndWhiteOverride=0; // Retail global0032FB00, separate from per-view mode.
};
struct View {bool active=true,target_active=true,in_game=true;int id=1;float near_z=.5f,far_z=500;
    float camera_focal_distance=0,camera_depth_of_field=0;Weather weather;
    int BlackAndWhiteMode=0;
    int scissor_x=0,scissor_y=0,scissor_w=512,scissor_h=448;};
struct PaletteResult {Palette linear{},clut{};bool enabled=false;};
struct Blend {std::uint8_t a=0,b=1,c=0,d=1,fix=128;
    std::uint64_t register_value()const{return a|(std::uint64_t(b)<<2)|(std::uint64_t(c)<<4)|(std::uint64_t(d)<<6)|(std::uint64_t(fix)<<32);}};
enum class Stage {DepthIntoAlpha,Fog,FocusMask,FocusClear,FocusJitter,FocusComposite,BlackAndWhite,RadialBlur,BlendPrevious,Filter};
struct Pass {Stage stage;unsigned render_stage=5;Blend blend;bool write_rgb=true,write_alpha=true;
    unsigned source_width=0,source_height=0,target_width=0,target_height=0;
    std::array<int,2> source_offset_fixed16{};
    std::uint64_t tex1=1,test=0x31001,zbuf=0x13a000000ULL;
};
struct Frame {PaletteResult fog,focus;std::vector<Pass> passes;};
struct ScreenFilter {bool bActive=false;std::array<std::uint8_t,4>aColour{{128,128,128,255}};
    Blend alpha{1,0,0,0,128};std::uint8_t nTextureFx=0;bool texture_available=false;};
struct ScreenControls {bool DrawScreenFilters=true,blend_mask_available=false,previous_frame_available=false;
    std::uint8_t nRadialAlphaCenter=0,nRadialAlphaEdge=192;ScreenFilter filter;};
struct Quad {std::array<std::array<float,2>,4> vertices{},uv{{{{0,0}},{{1,0}},{{1,1}},{{0,1}}}};
    std::array<std::array<std::uint8_t,4>,4> colours{};Blend blend;std::uint32_t texture_hash=0;
    std::uint8_t texture_fx=0;bool previous_frame=false;};
struct ScreenFrame {std::optional<Quad> filter,previous;std::vector<Quad> radial_mask;};
ScreenFrame prepare_screen(const View&,const ScreenControls&);
// Exact original default globals are checked against Retail0032FA98 and Debug00326B00.
Frame prepare(const View&,const Controls&,unsigned original_width,unsigned original_height);
// Retail eAttachDepthIntoAlpha00118A60: normal XYZF2 packed Z discards low4
// float-lane bits, then saturated Z16 high byte is copied to display alpha.
std::uint8_t depth_alpha_from_screen(float original_screen_z);
}
