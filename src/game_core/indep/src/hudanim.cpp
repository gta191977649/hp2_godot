// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/src/hudanim.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x0023B0F8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0023b0f8.md.
void HudAnimator::Sub_0023B0F8() {
    hp2::not_implemented("0x0023B0F8", "HudAnimator::Sub_0023B0F8 (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/indep/inc/HudAnim.hpp"
#include "game_core/port/driving/projections.hpp"
#include "game_core/port/driving/ee_scalar.hpp"
#include "game_core/src/bMath.hpp"
namespace port::race::detail {
// Native carriers for original explicit fields. Not an original object layout.
static void animation_vectors(driving::Memory& m,std::uint32_t a,
 const std::array<float,2>& pos,const std::array<float,2>& vec,
 const std::array<float,2>& scale,const std::array<float,2>& delta){
 for(unsigned k=0;k<2;++k){m.scalar(a+0x50+4*k,pos[k]);m.scalar(a+0x58+4*k,vec[k]);m.scalar(a+0x64+4*k,scale[k]);m.scalar(a+0x6c+4*k,delta[k]);}}
static void animation_pose(driving::Memory& m,std::uint32_t object,
 const std::array<float,2>& pos,const std::array<float,2>& scale){
 m.scalar(object+0x30,float(driving::cvt_w_s(pos[0])));m.scalar(object+0x34,float(driving::cvt_w_s(pos[1])));
 m.scalar(object+0x38,scale[0]);m.scalar(object+0x3c,scale[1]);}
static float animation_colour(float delta,float fraction,float start,const char* pc){
 return driving::ee_vadd_finite(driving::ee_vmula_finite(delta,fraction,pc).value,start,pc);}
static std::uint32_t animation_rgb(const std::array<float,3>& c){return
 std::uint32_t(driving::cvt_w_s(c[0]))|(std::uint32_t(driving::cvt_w_s(c[1]))<<8)|(std::uint32_t(driving::cvt_w_s(c[2]))<<16);}
}
namespace hp2::debug {
// Debug00234E38/00234EA8; E-0112-H02. Omitted constructor fields stay untouched.
void HudAnimator::Sub_00234E38(){for(auto off:{0x44u,0x110u,0x48u,0x4cu,0x40u,0x104u,0x108u,0x10cu})m_.word(address_+off,0);const std::array<float,4> c{{128,64,0,0}};for(unsigned k=0;k<4;++k){m_.scalar(address_+0xa0+4*k,c[k]);m_.scalar(address_+0xb0+4*k,0);}}
void HudAnimator::Sub_00234EA8(int flags){if(flags&1)m_.release(address_);}
// Debug00234EC8; E-0112-H02. Both nullable targets are visited independently.
void HudAnimator::SetVisibility(bool visible){if(auto p=m_.word(address_+0x48))FlailerObjectChunk(m_,p).SetVisibility(visible);if(auto p=m_.word(address_+0x4c))FlailerObjectChunk(m_,p).SetVisibility(visible);}
// Debug00234F20; E-0112-H02. Disassembly resolves decompiler's mis-typed
// stack colour/scale/string parameters; no ABI inference from float bit casts.
void HudAnimator::StartAnimation(FlailerTextChunk text,const std::array<float,2>& pos,const std::array<float,2>& vec,const std::array<float,2>& vec2,const std::array<float,2>& scale,const std::array<float,2>& delta,int alpha,float magnitude,const std::array<float,4>& colour,const std::array<float,2>& scale2,const std::array<float,2>& delta2,int alpha2,float magnitude2,const std::array<float,4>& colour2,float duration,float hold,float duration2,std::uint32_t string,std::uint32_t music){
 auto& m=m_;const auto a=address_;m.word(a+0x110,3);m.word(a+0x100,0);
 if(string){m.word(a+0x40,1);if(m.byte(0x328d58)!='%'||m.byte(0x328d59)!='s'||m.byte(0x328d5a))hp2::not_implemented("0x00234F20","original local string format differs from captured percent-s");unsigned i=0;for(;;++i){if(i>=64)hp2::not_implemented("0x00234F20","original local animation string exceeds64-byte storage");const auto c=m.byte(string+i);m.byte(a+i,c);if(!c)break;}}
 else{m.word(a+0x40,0);m.byte(a,0);}
 ::port::race::detail::animation_vectors(m,a,pos,vec,scale,delta);m.word(a+0x78,std::uint32_t(alpha));m.word(a+0x48,text.address());m.scalar(a+0x7c,magnitude);for(unsigned k=0;k<4;++k){m.scalar(a+0xd0+4*k,colour[k]);m.scalar(a+0xe0+4*k,colour2[k]);}for(unsigned k=0;k<2;++k){m.scalar(a+0x80+4*k,vec2[k]);m.scalar(a+0x88+4*k,scale2[k]);m.scalar(a+0x90+4*k,delta2[k]);}m.word(a+0x98,std::uint32_t(alpha2));m.scalar(a+0x9c,magnitude2);m.scalar(a+0xf0,duration);m.scalar(a+0xf4,hold);m.scalar(a+0xf8,duration2);m.word(a+0x10c,music);m.scalar(a+0xfc,float(m.signed_word(music?0x327cd8:0x327cf8))*m.scalar(0x32611c));m.word(a+0x104,1);if(text.address()){::port::race::detail::animation_pose(m,text.address(),pos,scale);text.SetAlpha(alpha);FlailerObjectChunk(m,text.address()).SetVisibility(1);}
}
// Debug002351D8; E-0112-H02. Timer argument unused in source; full WorldTimer.
void HudAnimator::StartAnimation(FlailerTextChunk text,const std::array<float,2>& pos,const std::array<float,2>& vec,const std::array<float,2>& scale,const std::array<float,2>& delta,int alpha,float magnitude,float duration,std::uint32_t t){
 (void)t;auto& m=m_;const auto a=address_;::port::race::detail::animation_vectors(m,a,pos,vec,scale,delta);m.word(a+0x78,std::uint32_t(alpha));m.word(a+0x48,text.address());m.word(a+0x110,1);m.scalar(a+0x7c,magnitude);
 const std::array<float,4> c{{128,64,0,0}},d{{0,64,128,0}};for(unsigned k=0;k<4;++k){m.scalar(a+0xa0+4*k,c[k]);m.scalar(a+0xb0+4*k,d[k]);}
 m.scalar(a+0xf0,duration);m.scalar(a+0xfc,float(m.signed_word(0x327cf8))*m.scalar(0x326120));m.word(a+0x104,1);::port::race::detail::animation_pose(m,text.address(),pos,scale);text.SetAlpha(alpha);FlailerObjectChunk(m,text.address()).SetVisibility(1);
}
// Debug00235320; E-0112-H02. AnimType changes even when already active.
void HudAnimator::StartAnimation(FlailerBitmapChunk bitmap,const std::array<float,2>& pos,const std::array<float,2>& vec,const std::array<float,2>& scale,const std::array<float,2>& delta,int alpha,float magnitude,float duration,std::uint32_t t,std::uint32_t visibility){
 (void)t;auto& m=m_;const auto a=address_;m.word(a+0x110,1);if(m.word(a+0x104))return;::port::race::detail::animation_vectors(m,a,pos,vec,scale,delta);m.word(a+0x78,std::uint32_t(alpha));m.scalar(a+0x7c,magnitude);m.scalar(a+0xf0,duration);m.scalar(a+0xfc,float(m.signed_word(0x327cf8))*m.scalar(0x326124));m.word(a+0x4c,bitmap.address());m.word(a+0x104,1);m.word(a+0x108,visibility);::port::race::detail::animation_pose(m,bitmap.address(),pos,scale);bitmap.SetAlpha(alpha);FlailerObjectChunk(m,bitmap.address()).SetVisibility(1);
}
// Debug00235440; E-0112-H02.
void HudAnimator::StartBlink(FlailerBitmapChunk bitmap,const std::array<float,4>& colour,const std::array<float,4>& magnitude,float rate,float delta){auto& m=m_;const auto a=address_;m.word(a+0x4c,bitmap.address());m.word(a+0x110,2);for(unsigned k=0;k<4;++k){m.scalar(a+0xa0+4*k,colour[k]);m.scalar(a+0xb0+4*k,magnitude[k]);}m.scalar(a+0xc0,rate);m.scalar(a+0xc4,delta);m.word(a+0x104,1);m.scalar(a+0xf0,0);m.scalar(a+0xfc,float(m.signed_word(0x327cf8))*m.scalar(0x326128));bitmap.SetAlpha(::port::driving::cvt_w_s(colour[3]));FlailerObjectChunk(m,bitmap.address()).SetVisibility(1);}
// Debug002354C8; E-0112-H02. Only upper fraction clamp, no invented lower clamp.
bool HudAnimator::DoAnimation(std::uint32_t t){(void)t;auto& m=m_;const auto a=address_;if(!m.word(a+0x104))return false;const auto duration=m.scalar(a+0xf0);if(duration==0)hp2::not_implemented("0x002354C8","zero animation duration requires exact EE exceptional division");float f=(float(m.signed_word(0x327cf8))*m.scalar(0x32612c)-m.scalar(a+0xfc))/duration;if(1<f)f=1;if(m.word(a+0x110)!=1)return false;
 std::array<float,2> pos,scale;for(unsigned k=0;k<2;++k){pos[k]=m.scalar(a+0x58+4*k)*f+m.scalar(a+0x50+4*k);scale[k]=m.scalar(a+0x6c+4*k)*f+m.scalar(a+0x64+4*k);}const int alpha=m.signed_word(a+0x78)+::port::driving::cvt_w_s(m.scalar(a+0x7c)*f);
 const auto text=m.word(a+0x48),bitmap=m.word(a+0x4c);if(text){::port::race::detail::animation_pose(m,text,pos,scale);FlailerTextChunk output(m,text);output.SetAlpha(alpha);std::array<float,3> c;for(unsigned k=0;k<3;++k)c[k]=::port::race::detail::animation_colour(m.scalar(a+0xb0+4*k),f,m.scalar(a+0xa0+4*k),"0x002354C8");output.SetColor(::port::race::detail::animation_rgb(c));if(1<=f){FlailerObjectChunk(m,text).SetVisibility(0);m.word(a+0x104,0);}}
 else if(bitmap){::port::race::detail::animation_pose(m,bitmap,pos,scale);FlailerBitmapChunk output(m,bitmap);output.SetAlpha(alpha);if(1<=f){output.SetAlpha(m.signed_word(a+0x78)+::port::driving::cvt_w_s(m.scalar(a+0x7c)));m.word(a+0x104,0);if(!m.word(a+0x108))FlailerObjectChunk(m,bitmap).SetVisibility(0);}}return false;
}
// Debug00235788; E-0112-H02. Does not test bActive in source.
bool HudAnimator::DoAnimation(std::uint32_t t,float intensity){(void)t;auto& m=m_;const auto a=address_,bitmap=m.word(a+0x4c);if(!bitmap)return false;float alpha=m.scalar(a+0xac);if(intensity==1)alpha+=m.scalar(a+0xbc);else if(intensity!=0){const float phase=hp2::recovered::bFMod(float(m.signed_word(0x327cf8))*m.scalar(0x326130)-m.scalar(a+0xfc),m.scalar(a+0xc0)+intensity*m.scalar(a+0xc4));if(m.word(a+0x110)!=2)return false;alpha=::port::race::detail::animation_colour(m.scalar(a+0xbc),phase,alpha,"0x00235788");}FlailerBitmapChunk(m,bitmap).SetAlpha(::port::driving::cvt_w_s(alpha));return false;}
// Debug00235898; E-0112-H02. At most one stage transition per call, carries
// excess time by incrementing StartTime by the completed stage duration.
bool HudAnimator::DoAnimation(const std::uint32_t* timer,std::uint32_t string){auto& m=m_;const auto a=address_;if(!m.word(a+0x104))return false;if(!timer)hp2::not_implemented("0x00235898","null original Timer pointer dereference");auto state=m.word(a+0x100);float fraction=float(std::int32_t(*timer))*m.scalar(0x326134)-m.scalar(a+0xfc);if(state<=2){const auto duration=m.scalar(a+(state==0?0xf0u:state==1?0xf4u:0xf8u));if(duration==0)hp2::not_implemented("0x00235898","zero stage duration requires exact EE exceptional division");fraction/=duration;}if(1<fraction)fraction=1;if(m.word(a+0x110)!=3)return false;if(state>2){state=0;m.word(a+0x100,0);}
 std::array<float,2> pos,scale;std::array<float,3> colour;int alpha;
 if(state==0){for(unsigned k=0;k<2;++k){pos[k]=m.scalar(a+0x58+4*k)*fraction+m.scalar(a+0x50+4*k);scale[k]=m.scalar(a+0x6c+4*k)*fraction+m.scalar(a+0x64+4*k);}alpha=m.signed_word(a+0x78)+::port::driving::cvt_w_s(m.scalar(a+0x7c)*fraction);for(unsigned k=0;k<3;++k)colour[k]=::port::race::detail::animation_colour(m.scalar(a+0xe0+4*k),fraction,m.scalar(a+0xd0+4*k),"0x00235898");}
 else{for(unsigned k=0;k<2;++k){pos[k]=state==1?m.scalar(a+0x58+4*k)+m.scalar(a+0x50+4*k):(m.scalar(a+0x80+4*k)*fraction+m.scalar(a+0x58+4*k))+m.scalar(a+0x50+4*k);scale[k]=state==1?m.scalar(a+0x88+4*k):m.scalar(a+0x90+4*k)*fraction+m.scalar(a+0x88+4*k);}alpha=m.signed_word(a+0x98);if(state==2)alpha+=::port::driving::cvt_w_s(m.scalar(a+0x9c)*fraction);for(unsigned k=0;k<3;++k)colour[k]=::port::driving::ee_vadd_finite(m.scalar(a+0xe0+4*k),m.scalar(a+0xd0+4*k),"0x00235898");}
 const auto text=m.word(a+0x48);if(!text)return false;::port::race::detail::animation_pose(m,text,pos,scale);FlailerTextChunk output(m,text);output.SetAlpha(alpha);output.SetString(m.word(a+0x40)?a:string);output.SetColor(::port::race::detail::animation_rgb(colour));if(1<=fraction){if(state==2){FlailerObjectChunk(m,text).SetVisibility(0);m.word(a+0x104,0);return true;}m.scalar(a+0xfc,m.scalar(a+0xfc)+m.scalar(a+(state==1?0xf4u:0xf0u)));m.word(a+0x100,state+1);}return false;
}
// Debug00235CA0; E-0112-H02. Blink restores alpha without clearing bActive;
// grow text/multi hide without forcing an endpoint pose.
void HudAnimator::FinishAnimation(){auto& m=m_;const auto a=address_,type=m.word(a+0x110),text=m.word(a+0x48),bitmap=m.word(a+0x4c);if(type==1){if(text){FlailerObjectChunk(m,text).SetVisibility(0);m.word(a+0x104,0);}else if(bitmap){std::array<float,2> pos,scale;for(unsigned k=0;k<2;++k){pos[k]=m.scalar(a+0x58+4*k)+m.scalar(a+0x50+4*k);scale[k]=m.scalar(a+0x6c+4*k)+m.scalar(a+0x64+4*k);}::port::race::detail::animation_pose(m,bitmap,pos,scale);FlailerBitmapChunk(m,bitmap).SetAlpha(m.signed_word(a+0x78)+::port::driving::cvt_w_s(m.scalar(a+0x7c)));m.word(a+0x104,0);if(!m.word(a+0x108))FlailerObjectChunk(m,bitmap).SetVisibility(0);}}else if(type==2){if(bitmap)FlailerBitmapChunk(m,bitmap).SetAlpha(::port::driving::cvt_w_s(m.scalar(a+0xac)));}else if(type==3&&text){FlailerObjectChunk(m,text).SetVisibility(0);m.word(a+0x104,0);}}
}
