// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\frontendscreens\feanim.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/AnimationGroup.hpp"
#include "game_core/port/frontend/popup.hpp"
#include "game_core/src/bMath.hpp"

namespace port::frontend {
// Debug00241FA0, E-0098-X04. Both original lookups consume scale X.
void PopupView::bind_animation(int index){
    target_=0xe0105663u+index;timer_=now_;
    const auto* w=hp2::frontend::FindWidget_00143D50(screen,target_,0);
    const float scale=w?w->scale_x:1.f;
    start_=scale*memory_.scalar(0x328e40);end_=scale*memory_.scalar(0x328e44);
}
// Debug00242040; E-0098-X04. Retains Debug sine and packed-timer arithmetic,
// while the host supplies RealTimer through the selection page's shared clock.
void PopupView::tick(std::uint32_t now,float dt){
    using namespace hp2::frontend;now_=now;
    // Debug0016AEB8 retains even the original unused arrow sine evaluation.
    arrow_phase_+=dt*9.f;
    if(memory_.scalar(0x3252c8)<arrow_phase_)arrow_phase_-=memory_.scalar(0x3252c8);
    (void)hp2::recovered::bSin(memory_,arrow_phase_);
    const float elapsed=float(std::uint32_t(now-timer_))*memory_.scalar(0x3261fc);
    if(elapsed<memory_.scalar(0x328e3c)){
        const float phase=(elapsed*memory_.scalar(0x326200))/memory_.scalar(0x328e3c);
        const float scale=start_+(end_-start_)*hp2::recovered::bSin(memory_,phase);
        const float alpha=memory_.scalar(0x328e48)+(memory_.scalar(0x328e4c)-memory_.scalar(0x328e48))*hp2::recovered::bSin(memory_,phase);
        auto* target=FindWidget_00143D50(screen,target_,1);auto* ghost=FindWidget_00143D50(screen,0xab671c9a,1);
        if(!target){if(ghost)ghost->visible=0;}
        else if(ghost){CopyWidgetTransform_001410A0(*ghost,*target);
            SetWidgetScale_00140FA8(*ghost,scale,measure_(*ghost));SetAlpha(*ghost,int(alpha));
            const auto colour=memory_.word(0x328a28);
            if(colour!=0xffffffffu)SetColor(*ghost,{std::uint8_t(colour),std::uint8_t(colour>>8),std::uint8_t(colour>>16)});
        }
    }
    if(memory_.scalar(0x328e50)<elapsed)timer_=now;
}
}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/feAnimAlphaElement.hpp"

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/inc/feAnim.hpp"

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/tMenuAnimElement.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x002486E8; E-0042-C163; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/002486e8.md.
void AnimationGroup::SetupToAnimate() {
    hp2::not_implemented("0x002486E8", "AnimationGroup::SetupToAnimate (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x00248568; E-0041-R11; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/00248568.md.
void AnimationGroup::Sub_00248568() {
    hp2::not_implemented("0x00248568", "AnimationGroup::Sub_00248568 (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/indep/src/frontendscreens/feanim.hpp"

namespace hp2::recovered {
// ELF 0x002491B0; E-0041-R19; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/002491b0.md.
void tAnimatedBlurb::Sub_002491B0() {
    hp2::not_implemented("0x002491B0", "tAnimatedBlurb::Sub_002491B0 (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x002490D8; E-0041-R12; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/002490d8.md.
void tExitScreenAnimation::Sub_002490D8() {
    hp2::not_implemented("0x002490D8", "tExitScreenAnimation::Sub_002490D8 (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x002495F0; E-0041-R10; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/002495f0.md.
void tGlobalBackgroundAnimation::Sub_002495F0() {
    hp2::not_implemented("0x002495F0", "tGlobalBackgroundAnimation::Sub_002495F0 (signature and full body unresolved)");
}

// ELF 0x00249790; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/00249790.md.
void tGlobalBackgroundAnimation::RemoveTextureHash() {
    hp2::not_implemented("0x00249790", "tGlobalBackgroundAnimation::RemoveTextureHash (signature and full body unresolved)");
}

// ELF 0x002497B8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002497b8.md.
void tGlobalBackgroundAnimation::GetNumTextures() {
    hp2::not_implemented("0x002497B8", "tGlobalBackgroundAnimation::GetNumTextures (signature and full body unresolved)");
}

// ELF 0x002497F0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002497f0.md.
void tGlobalBackgroundAnimation::GetNthTexture() {
    hp2::not_implemented("0x002497F0", "tGlobalBackgroundAnimation::GetNthTexture (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x0024A690; E-0041-R09; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/0024a690.md.
void tLeftRightArrowScroll::Sub_0024A690() {
    hp2::not_implemented("0x0024A690", "tLeftRightArrowScroll::Sub_0024A690 (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x00249C78; E-0041-R08; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/00249c78.md.
void tMenuAnimElement::Sub_00249C78() {
    hp2::not_implemented("0x00249C78", "tMenuAnimElement::Sub_00249C78 (signature and full body unresolved)");
}

// ELF 0x00249D70; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/00249d70.md.
void tMenuAnimElement::ApplyDeltaY() {
    hp2::not_implemented("0x00249D70", "tMenuAnimElement::ApplyDeltaY (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x00249DE0; E-0041-R07; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/00249de0.md.
void tScrollingMenuList::Sub_00249DE0() {
    hp2::not_implemented("0x00249DE0", "tScrollingMenuList::Sub_00249DE0 (signature and full body unresolved)");
}

// ELF 0x0024A050; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0024a050.md.
void tScrollingMenuList::FindMenuElement() {
    hp2::not_implemented("0x0024A050", "tScrollingMenuList::FindMenuElement (signature and full body unresolved)");
}

// ELF 0x0024A248; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0024a248.md.
void tScrollingMenuList::Highlight() {
    hp2::not_implemented("0x0024A248", "tScrollingMenuList::Highlight (signature and full body unresolved)");
}

// ELF 0x0024A3E0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0024a3e0.md.
void tScrollingMenuList::ShowUpArrow() {
    hp2::not_implemented("0x0024A3E0", "tScrollingMenuList::ShowUpArrow (signature and full body unresolved)");
}

// ELF 0x0024A430; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0024a430.md.
void tScrollingMenuList::ShowDownArrow() {
    hp2::not_implemented("0x0024A430", "tScrollingMenuList::ShowDownArrow (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x00248BE8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/00248be8.md.
void tSlideAnimation::Sub_00248BE8() {
    hp2::not_implemented("0x00248BE8", "tSlideAnimation::Sub_00248BE8 (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace port::selection::alpha_projection {
// Debug001411D8/00140308,00140FC8/00140CC0. Semantic adapter of existing widget storage.
unsigned get(const hp2::frontend::FeWidget& w,unsigned corner){
    if(w.type==2)return corner==0?w.a:w.field_0x5c[corner-1]>>24;
    return w.type==1?w.a:0;
}
void set(hp2::frontend::FeWidget& w,int alpha,unsigned corner){
    const auto value=static_cast<std::uint8_t>(alpha);
    if(w.type==2){
        w.dirty_0x2b=6;
        if(corner==0)w.a=value;
        else w.field_0x5c[corner-1]=(w.field_0x5c[corner-1]&0xffffffu)|(std::uint32_t(value)<<24);
    }else if(w.type==1 && w.a!=value){w.a=value;w.dirty_0x2b=6;}
}
}
namespace hp2::debug {
// Debug002416F8, E-0098-U03. Signed char conversion is intentional, including alpha128.
feAnimAlphaElement::feAnimAlphaElement(hp2::frontend::FeWidget* object):Object(object){
    if(Object)for(unsigned i=0;i<4;++i){const auto a=::port::selection::alpha_projection::get(*Object,i);
        SavedAlphas[i]=float(a<128?int(a):int(a)-256);}
}
// Debug002418A8, E-0098-U03. Always use captured alphas, never compound live alpha.
void feAnimAlphaElement::SetAlphaScale(float scale){
    if(!Object)return;
    const unsigned count=Object->type==2?4:1;
    for(unsigned i=0;i<count;++i)::port::selection::alpha_projection::set(*Object,int(SavedAlphas[i]*scale),i);
}
// Debug00241978, E-0098-U03. Restore authored corner alphas when leaving the screen.
feAnimAlphaElement::~feAnimAlphaElement(){SetAlphaScale(1);}
// Debug00241B00, E-0098-U03. Host screen reference replaces original screen-hash lookup.
feAlphaAnimation::feAlphaAnimation(hp2::frontend::FeScreen& screen):ScreenHash(screen){}
// Debug00241A30, E-0098-U03. Preserve original group traversal order.
feAlphaAnimation::feAlphaAnimation(hp2::frontend::FeScreen& screen,std::uint32_t group):ScreenHash(screen){
    for(auto& w:screen.widgets)if(w.class_hash==group)Fading.push_back(std::make_unique<feAnimAlphaElement>(&w));
}
// Debug00241B18, E-0098-U03. Missing original object is a no-op; duplicate additions are retained.
void feAlphaAnimation::AddObject(std::uint32_t hash){
    if(auto* w=hp2::frontend::FindWidget_00143D50(ScreenHash,hash,0))Fading.push_back(std::make_unique<feAnimAlphaElement>(w));
}
// Debug00241B90, E-0098-U03. No allocations in the frame update.
void feAlphaAnimation::SetAlphaScale(float scale){for(auto& item:Fading)item->SetAlphaScale(scale);}
// Debug00241BE8 ->00244080, E-0098-U03. Original removes head first; this matters
// when AddObject captured the same widget again after its alpha changed.
feAlphaAnimation::~feAlphaAnimation(){for(auto& item:Fading)item.reset();}
}

// TASK-0099 E-0099-W62, Debug authority815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
namespace hp2::debug {
void tHighlightAnimation::Sub_00241F40(int screen,int object,int color){
 m_.word(address_+8,0);m_.word(address_,static_cast<std::uint32_t>(screen));
 const auto timeout=m_.word(0x328e50),end=m_.word(0x328e44),start=m_.word(0x328e40);
 m_.word(address_+8,m_.word(0x327cd8));m_.word(address_+0xc,static_cast<std::uint32_t>(object));m_.word(address_+0x10,static_cast<std::uint32_t>(color));
 m_.word(address_+0x14,start);m_.word(address_+0x18,end);m_.word(address_+0x1c,timeout);m_.word(address_+4,0);
}
void tHighlightAnimation::Sub_00241F80(int flags){if(flags&1)m_.release(address_);}
}

#include "game_core/port/race/flailer_resources.hpp"
#include "game_core/port/driving/ee_scalar.hpp"
#include <cmath>
namespace hp2::debug {
// Debug00241FA0; E-0112-P02. Both original FlailerGetScale calls use ScaleX.
void tHighlightAnimation::SetCurrentMenuItem(int item){
 auto& m=m_;m.word(address_+4,std::uint32_t(item));m.word(address_+8,m.word(0x327cd8));
 float scale=1;const auto object=FindFlailerObject(m,m.word(address_),std::uint32_t(item),0);if(object)scale=m.scalar(object+0x38);
 m.scalar(address_+0x14,scale*m.scalar(0x328e40));m.scalar(address_+0x18,scale*m.scalar(0x328e44));
}
// Debug00242040; E-0112-P02. Source RealTimer, sine, copy/scale/alpha/color order.
void tHighlightAnimation::Animate(){
 auto& m=m_;const float elapsed=float(std::int32_t(m.word(0x327cd8)-m.word(address_+8)))*m.scalar(0x3261fc);
 if(elapsed<m.scalar(0x328e3c)){
  const float phase=(elapsed*m.scalar(0x326200))/m.scalar(0x328e3c);
  const float sine=hp2::recovered::bSin(m,phase);
  const float scale=m.scalar(address_+0x14)+(m.scalar(address_+0x18)-m.scalar(address_+0x14))*sine;
  const float alpha=m.scalar(0x328e48)+(m.scalar(0x328e4c)-m.scalar(0x328e48))*hp2::recovered::bSin(m,phase);
  const auto from=FindFlailerObject(m,m.word(address_),m.word(address_+4),1),to=FindFlailerObject(m,m.word(address_),m.word(address_+0xc),1);
  if(!from){if(to)m.byte(to+0x29,0);}
  else if(to){
   // Debug00140E90 CopyFlailerText preserves target's cached strip pointer.
   const auto strip=m.word(to+0x2c);for(unsigned i=0;i<0x40;++i)m.byte(to+0x28+i,m.byte(from+0x28+i));m.word(to+0x2c,strip);m.byte(to+0x2b,8);
   FlailerTextChunk text(m,to);text.ScaleAboutCenter(scale);
   if(!std::isfinite(alpha)||alpha<-2147483648.f||alpha>=2147483648.f)hp2::not_implemented("0x00242040","exceptional EE highlight alpha CVT.W.S operand");
   text.SetAlpha(::port::driving::cvt_w_s(alpha));if(m.word(address_+0x10)!=0xffffffffu)text.SetColor(m.word(address_+0x10));
  }
 }
 if(m.scalar(address_+0x1c)<elapsed)m.word(address_+8,m.word(0x327cd8));
}
// Debug002421B0; E-0112-P02. Other callback messages are original no-ops.
void tHighlightAnimation::HandleEvents(int msg,const ScreenCallbackData& d){if(msg==18)Animate();else if(msg==4)SetCurrentMenuItem(int(d.ItemHash));}
}
