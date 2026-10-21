#include "postprocess.hpp"
#include "game_core/src/ecstasy/eenveffectse.hpp"
#include "game_core/src/ecstasy/ecstasye.hpp"
#include "game_core/port/not_implemented.hpp"
#include <cstring>
#include <algorithm>
namespace port::track::postprocess {
std::uint8_t depth_alpha_from_screen(float screen_z){
    std::uint32_t bits;std::memcpy(&bits,&screen_z,4);
    const auto gs_z=(bits>>4)&0xffffffu;
    return std::uint8_t(std::min(255u,gs_z>>8));
}
ScreenFrame prepare_screen(const View& view,const ScreenControls& controls){
    if(!controls.DrawScreenFilters)return {};
    return {hp2::recovered::DisplayFilter(view,controls),hp2::recovered::DisplayBlendPrevious(view,controls),
        hp2::recovered::DisplayRadialBlur(view,controls)};
}
// Port orchestration of Retail eDisplayFrame00112858 stage5. Controls carry
// original globals, not host quality settings. Stage7 additions are separate.
Frame prepare(const View& view,const Controls& controls,unsigned width,unsigned height){
    Frame frame;if(!view.in_game||!view.active||!view.target_active)return frame;
    if(width!=512||height==0||height>512)
        hp2::not_implemented("0x00118A60","Postprocess packet layout outside original512-wide target");
    if(hp2::recovered::eAttachDepthIntoAlpha(view,controls,frame,width,height)){
        hp2::recovered::eAttachFog(view,controls,frame,width,height);
        hp2::recovered::eAttachDepthOfField(view,controls,frame,width,height);
    }
    hp2::recovered::eAttachBlackAndWhite(view,controls,frame,width,height);
    return frame;
}
}
