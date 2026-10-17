#include "game_core/port/track/effects.hpp"
#include <iostream>
#include <stdexcept>
namespace {void check(bool b){if(!b)throw std::runtime_error("Track effect reference mismatch");}}
int main(){try{
    using namespace port::track;
    // GS equations exercise extrapolation above unity alpha and Ad/As distinction.
    check(blend_channel(100,40,192,64,0x44)==130);
    check(blend_channel(100,40,192,64,0x48)==190);
    check(blend_channel(100,40,192,64,0x58)==90);
    check(blend_channel(100,40,192,64,0x0a)==100);
    check(blend_channel_gs(0,1,1,0,0x44)==0);
    check(blend_channel_gs(255,255,255,255,0x48)==763);
    const std::vector<AlphaKey> keys{{2,10},{1,1},{2,30},{2,30},{-1,-4},{7,-2}};
    check(alpha_order_001055B0(keys)==std::vector<std::size_t>({1,3,2,0,5,4}));
    // Near/far endpoints and true source matrix-pack ranges, with no world shift.
    check(screen_depth(1,1,101,0)==screen_depth_range(0)[1]);
    check(std::abs(screen_depth(101,1,101,1)-screen_depth_range(1)[0])<1e-6f);
    check(screen_depth_to_gs(screen_depth_range(0)[0])==0x40000010);
    check(screen_depth_to_gs(screen_depth_range(1,true)[1],0x31)==0x802000);
    check(normalized_gpu_depth(0xffffff,0x31)==1.0);
    check(std::abs(standard_host_depth(1,1,101,0)-1.0)<1e-6);
    check(std::abs(standard_host_depth(101,1,101,0))<1e-6);
    check(standard_host_depth(10,1,101,1)>standard_host_depth(10,1,101,0));
    port::vehicle::VuMaterialBlock m{};port::vehicle::VuLightBlock l{};
    m[0][2]=1;m[3]={{0,1065353216.f,80,120}};
    check(alpha_002A6F78({{0,0,1}},m,l)==200);
    m[3][2]=250;check(alpha_002A6F78({{0,0,1}},m,l)==255);
    // Shipped GLOBALB Road registry parameter projection, all30 track40 materials.
    // Original p13=1.6 means power at dot1 exceeds1, even with unit directions.
    m[0][2]=.1f;m[3]={{8,-7452439552.f,20,108}};l[4][0]=1;
    check(alpha_002A6F78({{1,0,0}},m,l)==192);
    hp2::assets::TpkTextureHeader h;h.Field_0x6f=1;h.Field_0x70=3;h.alpha_bits=0x48;h.BiasLevel=1;h.Field_0x50=1;h.Field_0x4d=1;
    auto e=effect_state(h,0x4041);check(e.rewrite_alpha&&e.second_pass_320&&e.test.write_depth&&e.matrix_pack==1&&e.alpha_pass);
    h.is_any_semitransparency=1;e=effect_state(h,0x4041);check(can_fuse_additive_40(e));
    const auto passes=pass_sequence(e,123);check(passes.size()==2&&passes[1].texture_hash==0x0184d1f8&&passes[1].alpha_register==0x4000000058ULL);
    h.is_any_semitransparency=0;h.alpha_bits=0x0a;e=effect_state(h,0x4041);
    check(fusion_40(e,127,127,127,127)==Fusion40::Copy);
    check(fusion_40(e,0,127,127,127)==Fusion40::None);
    check(fused_copy_40_source(197,239,192)==255);
    // Exhaust all destination byte values and multiple above-unity source-alpha
    // cases: sequential saturation equals the fused nonnegative sum.
    for(unsigned dest=0;dest<256;++dest)for(unsigned as:{0u,1u,127u,128u,192u,255u}){
        const int base=std::min(255,blend_channel_gs(197,std::uint8_t(dest),std::uint8_t(as),0,0x48));
        const int second=std::min(255,blend_channel_gs(239,std::uint8_t(base),0,std::uint8_t(as),0x58));
        check(second==std::min(255u,dest+fused_additive_40_increment(197,239,std::uint8_t(as))));
    }
    check(SecondPass320{}.alpha==0x4000000058ULL);
    std::cout<<"Track effect references passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
