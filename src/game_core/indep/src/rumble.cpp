// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/src/rumble.hpp"

#include "game_core/port/not_implemented.hpp"
#include "game_core/port/driving/projections.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/src/ecstasy/emathe.hpp"
#include "game_core/indep/inc/Player.hpp"
#include <cstring>
#include <cmath>

namespace hp2::debug {
// Debug00155500, E-0099-W50. Exact point interval and last-index selection.
int tEnvelope::GetIndex(float time){int index=0;const auto count=m_.signed_word(address_+4);const auto points=m_.word(address_);if(count>0){if(time<m_.scalar(points))return 0;index=1;while(index<count&&m_.scalar(points+std::uint32_t(index)*8)<=time)++index;}return index-(index>0?1:0);}
// Debug00155560, E-0099-W50. Last endpoint is outside the actual envelope.
bool tEnvelope::OutOfRange(float time){const auto points=m_.word(address_);const auto count=m_.signed_word(address_+4);if(!points||count<=0)hp2::not_implemented("0x00155560","original tEnvelope point source is not initialized");return time<m_.scalar(points)||m_.scalar(points+std::uint32_t(count-1)*8)<=time;}
// Debug001555A8, E-0099-W50. Out-of-segment slope is genuinely zero.
float tEnvelope::GetSlope(int index){if(index<0||m_.signed_word(address_+4)-2<index)return 0;const auto point=m_.word(address_)+std::uint32_t(index)*8;const float duration=m_.scalar(point+8)-m_.scalar(point);if(duration==0)hp2::not_implemented("0x001555A8","degenerate original envelope time interval division");return (m_.scalar(point+12)-m_.scalar(point+4))/duration;}
// Debug00155658, E-0099-W50. Original range, slope then interpolation order.
float tEnvelope::GetValue(float time){if(OutOfRange(time))return 0;const int index=GetIndex(time);const float slope=GetSlope(index);const auto point=m_.word(address_)+std::uint32_t(index)*8;return m_.scalar(point+4)+slope*(time-m_.scalar(point));}
// Debug00155E20, E-0099-W50. Amplitude has no alternating phase gate.
float tShaker::GetAmplitude(){const auto envelope=m_.word(address_+0x10);return envelope?tEnvelope(m_,envelope).GetValue(m_.scalar(address_+0x14)):0;}
// Debug00155E58, E-0099-W50. Original vector negation retains every lane.
void tShaker::StartShaking(const std::array<float,4>& value,float duration,float frequency){auto shake=value;for(auto& lane:shake)lane*=-1;m_.vector(address_,shake);m_.scalar(address_+0x1c,frequency);m_.word(address_+0x10,0x327400);m_.scalar(address_+0x18,duration);m_.scalar(address_+0x14,0);}
// Debug00155E90, E-0099-W50. Original normalized-time advance and source release.
void tShaker::Update(float elapsed){const auto envelope=m_.word(address_+0x10);if(envelope){const auto time=m_.scalar(address_+0x14)+elapsed/m_.scalar(address_+0x18);m_.scalar(address_+0x14,time);if(tEnvelope(m_,envelope).OutOfRange(time))m_.word(address_+0x10,0);}}
// Debug00155EE0, E-0099-W50. Exact even-phase pulse; inactive source yields
// original zero gain without constructing or substituting an empty envelope.
void tShaker::GetValue(std::array<float,4>& output){const auto phase=std::uint32_t(::port::driving::cvt_w_s(m_.scalar(address_+0x14)*m_.scalar(address_+0x1c)));float amplitude=0;const auto envelope=m_.word(address_+0x10);if((((phase^1u)&1u)!=0)&&envelope)amplitude=tEnvelope(m_,envelope).GetValue(m_.scalar(address_+0x14));output=m_.vector(address_);for(auto& lane:output)lane*=amplitude;}
// Debug00156478, E-0099-W50. Complete observed original process-static body;
// both-port rumble objects and camera shakers use their actual authored tables.
void Sub_00156478(::port::driving::Memory& m){m.scalar(0x2f1f68,m.scalar(0x324f88));m.scalar(0x32b010,-1200);m.scalar(0x2f1f60,0);m.scalar(0x32b000,m.scalar(0x324f8c));m.scalar(0x32b018,0);m.scalar(0x2f1f64,0);m.scalar(0x32b008,2);m.scalar(0x32b014,2000);m.scalar(0x32b004,.5f);
 const std::uint32_t envelopes[]={0x3273c8,0x3273d0,0x3273d8,0x3273e0,0x3273e8,0x3273f0,0x3273f8,0x327400},points[]={0x2f1f70,0x2f1f90,0x2f1fb0,0x2f1fc8,0x2f1fe0,0x2f1ff0,0x2f2000,0x2f2020},counts[]={4,4,3,3,2,2,4,5};for(unsigned i=0;i<8;++i){m.word(envelopes[i],points[i]);m.word(envelopes[i]+4,counts[i]);}
 for(unsigned i=0;i<4;++i){const auto rumble=0x2f2048+i*20;m.word(rumble,0);m.scalar(rumble+4,0);m.scalar(rumble+8,1);m.word(rumble+12,0);m.word(rumble+16,0);}for(unsigned i=0;i<2;++i){const auto shaker=0x2f20a0+i*0x20;m.scalar(shaker,0);m.scalar(shaker+4,0);m.scalar(shaker+8,0);m.word(shaker+0x10,0);m.scalar(shaker+0x14,0);m.scalar(shaker+0x18,1);m.scalar(shaker+0x1c,17);}
}
// Debug00155F70, E-0099-W50. Actual debugger flag and both camera components.
void UpdateCameraShakers(::port::driving::Memory& m){if(m.word(0x3273c0)){m.word(0x3273c0,0);tShaker(m,0x2f20a0).StartShaking(m.vector(0x2f1f60),m.scalar(0x324f7c),17);}for(unsigned i=0;i<2;++i)tShaker(m,0x2f20a0+i*0x20).Update(m.scalar(0x327d08));}
// Debug00156048, E-0099-W50. Original player-indexed shaker, no copied state.
void GetShake(::port::driving::Memory& m,int player,std::array<float,4>& output){if(player<0||player>=2)throw std::out_of_range("Original CameraShakers has two entries");tShaker(m,0x2f20a0+std::uint32_t(player)*0x20).GetValue(output);}
// Debug00156080, E-0099-W50. Original timescale/view gates and full transform
// chain execute even for zero shake; never substitute an identity/no-op branch.
void ApplyCameraShake(::port::driving::Memory& m,int view,std::array<float,16>& matrix){const auto world=m.word(0x3272f0);if(!world)hp2::not_implemented("0x00156080","original camera shake requires current World timescale");if(m.scalar(world+0x184)<1)return;int player;if(view==2)player=1;else{if(view!=1&&view!=3)return;player=0;}std::array<float,4> shake{};GetShake(m,player,shake);shake[3]=0;auto transform=hp2::recovered::bIdentity();std::array<float,4> translated{};eMulVector(translated,matrix,shake);translated[3]=1;for(unsigned lane=0;lane<4;++lane)transform[12+lane]=translated[lane];hp2::recovered::eRotateX(m,transform,transform,std::uint16_t(::port::driving::cvt_w_s(m.scalar(0x32b010)*(transform[14]+transform[13]))));hp2::recovered::eRotateY(m,transform,transform,std::uint16_t(::port::driving::cvt_w_s(m.scalar(0x32b014)*transform[12])));hp2::recovered::eRotateZ(m,transform,transform,std::uint16_t(::port::driving::cvt_w_s(m.scalar(0x32b018)*(transform[12]+transform[13]))));hp2::recovered::eMulMatrix(matrix,matrix,transform);}
}

namespace hp2::recovered {
// ELF 0x00155948; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00155948.md.
void tEnvelope::GetIndex() {
    hp2::not_implemented("0x00155948", "tEnvelope::GetIndex (signature and full body unresolved)");
}

// ELF 0x001559A8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001559a8.md.
void tEnvelope::OutOfRange() {
    hp2::not_implemented("0x001559A8", "tEnvelope::OutOfRange (signature and full body unresolved)");
}

// ELF 0x001559F0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001559f0.md.
void tEnvelope::GetSlope() {
    hp2::not_implemented("0x001559F0", "tEnvelope::GetSlope (signature and full body unresolved)");
}
} // namespace hp2::recovered
namespace hp2::debug {
// Debug00155820, E-0099-W38. Original leaves nValue/nValuePrevious untouched.
void tRumbler::StartRumble(std::uint32_t envelope,float amplitude){m_.word(address_,envelope);m_.scalar(address_+8,amplitude);m_.scalar(address_+4,0);}
// Debug001559F8, E-0099-W38. Actual RC/flow/settings/overflow/priority gates.
// PortRumblers is imported tRumbler[2][2],20-byte elements; out-of-domain ports
// would overrun the source global array and are a port bounds error.
void PortRumble(::port::driving::Memory& m,int port,float intensity,std::uint32_t coefficients,std::uint32_t envelope,bool overflow){
 if(port<0||port>=2)throw std::out_of_range("Original PortRumblers has2 port rows");
 if(m.word(0x3275a8)){const auto coordinator=m.word(0x327290);if(!coordinator)hp2::not_implemented("0x001559F8","Original in-game PortRumble requires current RaceCoordinator");const auto state=m.word(coordinator);if(state!=6&&state!=3)return;}
 float minimum=m.scalar(coefficients);if(intensity<minimum)return;float maximum=m.scalar(coefficients+4);const auto base=0x2f2048+static_cast<std::uint32_t>(port)*0x28;
 const auto enabled=[&](){if(m.word(0x3275ac)==3)return m.byte(0x2f3890+0x10738+static_cast<std::uint32_t>(port!=0)*0x6d0+0x645)!=0;const auto player=m.word(0x327770+static_cast<std::uint32_t>(port!=0)*4);return player&&m.byte(player+0x1d)!=0;};
 if(maximum<intensity){intensity=maximum;if(overflow&&enabled()){tRumbler(m,base+20).StartRumble(0x3273e8,1);minimum=m.scalar(coefficients);maximum=m.scalar(coefficients+4);}}
 const float range=maximum-minimum;if(range==0)hp2::not_implemented("0x00155B38","Degenerate original rumble coefficient division is not modeled");const float amplitude=((intensity-minimum)/range)*(m.scalar(coefficients+12)-m.scalar(coefficients+8))+m.scalar(coefficients+8);
 // Fresh MIPS compares new<old and returns, rather than using the decompiler's
 // inverted <= expression; equal amplitude is allowed to replace the envelope.
 if(!overflow&&m.word(base)&&amplitude<m.scalar(base+8))return;
 if(enabled())tRumbler(m,base).StartRumble(envelope,amplitude);
}
// Debug00155C40, E-0099-W38. This is actual game queue state, not a host event.
void PortRumbleImpact(::port::driving::Memory& m,int port,float intensity){m.word(0x327408,0x327410);PortRumble(m,port,intensity,0x2a92f0,0x3273c8,true);}
// E-0099-W66: the first point is changed before PortRumble's range/gates.
void PortRumbleShockSway(::port::driving::Memory& m,int port,float intensity){const auto envelope=port==0?0x3273d8u:0x3273e0u;m.word(0x327408,0x2a9280);const auto c=0x2a9300u;const float width=m.scalar(c+4)-m.scalar(c);if(width==0)hp2::not_implemented("0x00155C60","degenerate original rumble coefficient interval");m.scalar(m.word(envelope)+4,((intensity-m.scalar(c))/width)*(m.scalar(c+20)-m.scalar(c+16))+m.scalar(c+16));PortRumble(m,port,intensity,c,envelope,false);}
void PortRumbleRoadNoise(::port::driving::Memory& m,int port,float intensity){const auto envelope=port==0?0x3273d8u:0x3273e0u;m.word(0x327408,0x2a9290);const auto c=0x2a9318u;const float width=m.scalar(c+4)-m.scalar(c);if(width==0)hp2::not_implemented("0x00155CB8","degenerate original rumble coefficient interval");m.scalar(m.word(envelope)+4,((intensity-m.scalar(c))/width)*(m.scalar(c+20)-m.scalar(c+16))+m.scalar(c+16));PortRumble(m,port,intensity,c,envelope,false);}
void PortRumbleShockBottom(::port::driving::Memory& m,int port,float intensity){m.word(0x327408,0x2a92a0);PortRumble(m,port,intensity,0x2a9330,0x3273c8,true);}
void PortRumbleBlownTires(::port::driving::Memory& m,int port,float intensity){m.word(0x327408,0x2a92b0);PortRumble(m,port,intensity,0x2a9340,0x3273d0,true);}
void PortRumbleSingleRoadBump(::port::driving::Memory& m,int port,float intensity){m.word(0x327408,0x2a92c0);PortRumble(m,port,intensity,0x2a9350,0x3273f0,true);}
void PortRumbleDoubleRoadBump(::port::driving::Memory& m,int port,float intensity){m.word(0x327408,0x2a92d8);PortRumble(m,port,intensity,0x2a9350,0x3273f8,true);}
}

namespace hp2::debug {
// Debug001561E0, E-0099-W58. Source XYZ bias/dot/clamp/interpolation and
// actual per-player shaker. The local vShake.W is never written by bNormalize;
// an explicit port operand is required only when the original full-vector copy
// is reached. No implicit zero or assertion about the original stack is made.
void MaybeCameraShake(::port::driving::Memory& m,int player,std::uint32_t acceleration,std::optional<std::uint32_t> port_stack_shake_w){
 if(player<0||player>=2)throw std::out_of_range("Original camera shake has two player rows");
 const auto owner=m.word(0x327770+std::uint32_t(player)*4);
 if(!owner)hp2::not_implemented("0x001561E0","original camera shake requires Player owner");
 if(Player(m,owner).IsCameraPOVInCar())return;
 std::array<float,3> biased{};for(unsigned i=0;i<3;++i){biased[i]=m.scalar(0x32b000+4*i)*m.scalar(acceleration+4*i);if(!std::isfinite(biased[i]))hp2::not_implemented("0x00156240","exceptional EE camera-shake bias operands");}
 const float xy=biased[0]*biased[0]+biased[1]*biased[1];float intensity=std::sqrt(xy+biased[2]*biased[2]);
 if(!std::isfinite(intensity))hp2::not_implemented("0x00156278","exceptional EE camera-shake dot/sqrt operands");
 const auto coefficients=0x2a9360u;const float minimum=m.scalar(coefficients);if(intensity<minimum)return;
 const float maximum=m.scalar(coefficients+4);if(maximum<intensity)intensity=maximum;
 if(maximum==minimum)hp2::not_implemented("0x001562C4","degenerate original camera-shake coefficient interval");
 const float fraction=(intensity-minimum)/(maximum-minimum);
 float amplitude=m.scalar(coefficients+8)+fraction*(m.scalar(coefficients+12)-m.scalar(coefficients+8));
 tShaker shaker(m,0x2f20a0+std::uint32_t(player)*0x20);if(!(shaker.GetAmplitude()<amplitude))return;
 float duration=m.scalar(coefficients+16)+fraction*(m.scalar(coefficients+20)-m.scalar(coefficients+16));
 if(m.word(owner+0x54)==7){amplitude*=m.scalar(0x324f80);duration*=m.scalar(0x324f84);}
 if(!port_stack_shake_w)hp2::not_implemented("0x00156350","original vShake.W is unwritten; explicit port_stack_shake_w operand required");
 const auto xyz=hp2::recovered::bNormalize(biased,amplitude);float w;const auto bits=*port_stack_shake_w;std::memcpy(&w,&bits,4);
 shaker.StartShaking({{xyz[0],xyz[1],xyz[2],w}},duration,17);
}
}
