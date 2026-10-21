#include "water_effects.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/port/not_implemented.hpp"
namespace port::driving {
// Debug00133590, E0089W02; signed comparisons and 82-frame rounding.
void turn_wipers_on_for_duration(Memory& m,std::uint32_t car,std::int32_t start,std::int32_t end){
 if(m.signed_word(car+0x840)!=0)return;
 auto on=m.signed_word(car+0x838),off=m.signed_word(car+0x83c);
 const auto now=m.signed_word(0x327d00);
 if(on<now){if(off<=now){on=start;m.word(car+0x838,on);}}
 else if(start<on){on=start;m.word(car+0x838,on);}
 if(off<end)off=end;
 // Explicit two's complement arithmetic, including original wrap semantics.
 const auto delta=static_cast<std::int32_t>(std::uint32_t(off)-std::uint32_t(on)+81u);
 m.word(car+0x83c,std::uint32_t(delta/82)*82u+std::uint32_t(on));
}
WiperSnapshot wiper_snapshot(const Memory& m,std::uint32_t car){
 return {m.signed_word(car+0x840),m.signed_word(car+0x838),m.signed_word(car+0x83c),m.word(0x327d00)};
}
// Debug00133988 water slice, E0089W01. Sound object ownership is port-side.
void trigger_water_terrain_state(Memory& m,std::uint32_t car,float distance,SplashAudioMixer& audio,
 const std::function<void(float,const char*)>& replay){
 unsigned mask=0;bool deep=false;
 for(unsigned wheel=0;wheel<4;++wheel)if(m.byte(car+0x350+wheel)){
  const auto terrain=m.word(car+0x354+wheel*4);
  if(terrain==4||terrain==5||terrain==20){mask|=1u<<wheel;deep|=terrain==5;}
 }
 const auto speed=m.scalar(car+0x3c4);
 if(distance<60&&mask&&speed>10){
  const auto now=m.word(0x327d00);
  turn_wipers_on_for_duration(m,car,static_cast<std::int32_t>(now+60),static_cast<std::int32_t>(now+180));
  if(m.word(car+0x850))audio.trigger((mask&6)!=0,deep?3:1);
  if(m.word(car+0x18)&&((m.word(0x327d10)+3u)&3u)==0&&speed>30){
   if(!deep||m.word(car+0x88c)==0){
    if(!replay)hp2::not_implemented("0x0013E070","Water replay event service missing");
    replay(deep?10.f:1.25f,deep?"InDeepWater":"InShallowWater");
    if(deep)m.word(car+0x88c,1);
   }
  }
 }
 if(!deep)m.word(car+0x88c,0);
}
// BaseCarSound::UpdateSplash0018B4A8 + GetSplashSample0018B958,
// sound::Random001ACF88; E0089W03. Left then right as Update0018B348.
std::vector<SplashAudioRequest> SplashAudioMixer::update(Memory& m,float volume){
 std::vector<SplashAudioRequest> out;
 for(unsigned side=0;side<2;++side){
  auto& timer=timers_[side];auto& intensity=intensities_[side];
  if(timer!=0)--timer;
  if(intensity!=0){
   if(timer==0){
    if(m.word(0x326f40)&&m.word(0x327980)&&m.word(0x3289b8))
     out.push_back({hp2::recovered::bRandom(m,0x327eec,std::uint32_t(3)),volume*float(intensity),0});
    timer=10;
   }
   intensity=0;
  }
 }
 return out;
}
}
