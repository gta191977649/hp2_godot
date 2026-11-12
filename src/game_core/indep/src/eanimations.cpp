// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\eanimations.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/eAnimation.hpp"
#include "game_core/port/race/car_render_resources.hpp"
#include "game_core/port/not_implemented.hpp"

namespace hp2::debug {
// Debug001A4990, E-0099-W28. Original process-static initializer.
void Sub_001A4990(::port::driving::Memory& m){m.scalar(0x3087e8,1);m.word(0x327d9c,0x327d98);m.scalar(0x3087e0,1);m.word(0x327d98,0x327d98);m.scalar(0x3087e4,1);}
// Debug001A3FD8, E-0099-W28. Model is an actual embedded24-byte eModel.
void eAnimation::Sub_001A3FD8(){
 const auto tail=m_.word(0x327d9c);if(!tail)hp2::not_implemented("0x001A4990","original animation process static initialization has not run");
 m_.word(tail,address_);m_.word(address_+0x10,0);m_.word(0x327d9c,address_);m_.word(address_+0x14,0);
 m_.scalar(address_+0x38,1);m_.word(address_+4,tail);m_.word(address_,0x327d98);
 m_.word(address_+0x20,0);m_.word(address_+0x24,0);m_.word(address_+0x28,0);m_.word(address_+0x2c,0);
 m_.scalar(address_+0x30,0);m_.scalar(address_+0x34,0);m_.word(address_+0x40,0);m_.word(address_+0x44,0);m_.word(address_+0x48,0);
}
// Debug001A4080/001A4088 are original inlined eModel Init/UnInit operations.
void eAnimation::Init(std::uint32_t hash){::port::race::eModel(m_,address_+8).Init(hash);}
void eAnimation::UnInit(){::port::race::eModel(m_,address_+8).UnInit();}
// Debug001A4030, E-0099-W28. Detach animation, embedded model, then object.
void eAnimation::Sub_001A4030(int flags){const auto next=m_.word(address_),previous=m_.word(address_+4);m_.word(previous,next);m_.word(next+4,previous);UnInit();if(flags&1)m_.release(address_);}
}
#include "game_core/port/race/nis_resources.hpp"
namespace hp2::debug {
// Debug001A53E8; actual independent original list static initializer.
void Sub_001A53E8(::port::driving::Memory& m){m.word(0x327dac,0x327da8);m.word(0x327da8,0x327da8);}
std::uint32_t eFindAnimationSolid(::port::driving::Memory& m,std::uint32_t hash){for(auto a=m.word(0x327da8);a!=0x327da8;a=m.word(a))if(m.word(a+0x40)==hash)return a;return 0;} //001A4EA8.
int LoaderAnimations(::port::driving::Memory& m,std::uint32_t chunk){
 if(m.word(chunk)!=0x80034008)return 0;
 const auto end=chunk+8+m.word(chunk+4),first=chunk+8,solid=(first+0x17)&~15u;
 const auto rotations=first+8+m.word(first+4);m.word(solid+0x4c,rotations+8);
 const auto translations=rotations+8+m.word(rotations+4);m.word(solid+0x54,translations+8);
 // Source loop copies each16-byte key to an unused stack local; retain its
 // source reads without assigning invented decoded-animation behavior.
 for(unsigned i=0;i<m.half(solid+0x44);++i)for(unsigned j=0;j<4;++j)(void)m.word(translations+8+i*16+j*4);
 const auto labels=translations+8+m.word(translations+4);if(labels!=end)m.word(solid+0x58,labels+8);
 (void)eFindAnimationSolid(m,m.word(solid+0x40));const auto previous=m.word(0x327dac);m.word(0x3267fc,1);m.word(previous,solid);m.word(0x327dac,solid);m.word(solid,0x327da8);m.word(solid+4,previous);return 1;
}
int UnloaderAnimations(::port::driving::Memory& m,std::uint32_t chunk){if(m.word(chunk)!=0x80034008)return 0;const auto next=m.word(chunk+16),previous=m.word(chunk+20);m.word(0x3267fc,1);m.word(previous,next);m.word(next+4,previous);return 1;} //001A4E68.
void eAnimation::SetAnimation(std::uint32_t hash,float frame){if(m_.word(address_+0x20)!=hash){m_.word(address_+0x20,hash);m_.word(address_+0x28,eFindAnimationSolid(m_,hash));}m_.scalar(address_+0x30,frame);m_.scalar(address_+0x38,1);} //001A4148.
}
#include <cmath>
namespace hp2::debug {
// Debug001A4730. Original animation union plus largest model absolute extent;
// native output pointers are bVector3: caller W remains untouched.
void eAnimation::GetBoundingBox(std::array<float,4>& low,std::array<float,4>& high)const{const auto first=m_.word(address_+0x28);if(!first)return;for(unsigned i=0;i<3;++i)low[i]=high[i]=m_.scalar(first+0x20+4*i);const auto expand=[&](std::uint32_t at){for(unsigned i=0;i<3;++i){const float value=m_.scalar(at+4*i);if(value<low[i])low[i]=value;if(high[i]<value)high[i]=value;}};expand(first+0x30);if(const auto other=m_.word(address_+0x2c)){expand(other+0x20);expand(other+0x30);}std::array<float,4> model_low{{0,0,0,0}},model_high{{0,0,0,0}};::port::race::eModel(m_,address_+8).GetBoundingBox(model_low,model_high);float radius=std::fabs(model_low[0]);for(float value:{model_low[1],model_low[2],model_high[0],model_high[1],model_high[2]}){value=std::fabs(value);if(!std::isfinite(value)||!std::isfinite(radius))hp2::not_implemented("0x001A4730","exceptional EE model-bound maximum is not reviewed");if(value>=radius)radius=value;}for(unsigned i=0;i<3;++i){low[i]-=radius;high[i]+=radius;}}
}
