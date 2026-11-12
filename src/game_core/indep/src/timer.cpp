#include "game_core/indep/src/timer.hpp"

// TASK-0042: bounded host projections; original member ABI remains unverified.
#include "game_core/port/driving/projection_support.hpp"
// Retail 0x001A33C0; E-0041-M01; E-0042-S001; exact Debug TU ..\indep\src\timer.cpp
namespace port::driving::native {
std::uint32_t GetQuantizedRealTimeElapsed(float value){return static_cast<std::uint32_t>(hp2::recovered::GetQuantizedRealTimeElapsed(value));}
}

#include "game_core/indep/src/timer.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x001A3B28; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/001a3b28.md.
void Timer::GetHoursMinsSeconds() {
    hp2::not_implemented("0x001A3B28", "Timer::GetHoursMinsSeconds (signature and full body unresolved)");
}

// ELF 0x001A3CF0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001a3cf0.md.
void Timer::CountDown() {
    hp2::not_implemented("0x001A3CF0", "Timer::CountDown (signature and full body unresolved)");
}
} // namespace hp2::recovered

// TASK-0044 E-0044-P05, Debug timer.cpp. Original static global operands retained.
namespace hp2::recovered {
// Debug001A0C08.
float GetRealTimeElapsedFromQuantized(port::driving::Memory& m,std::int32_t v){return static_cast<float>(v)*m.scalar(0x003256c8);}
// Debug001A0C20.
std::int32_t GetQuantizedRealTimeElapsed(float v){v=v*3600.0f;v=v+0.5f;return port::driving::cvt_w_s(v);}
namespace {std::uint32_t packed_time(float v){v=v*4000.0f;v=v+0.5f;return static_cast<std::uint32_t>(port::driving::cvt_w_s(v));}}
// Debug001A0C50. Joylog services bind original replay/observation calls.
void PrepareRealTimestep(port::driving::Memory& m,float video,port::driving::native::ScheduleServices& calls){
 const auto index=m.word(0x00327cf4)&15u;std::uint32_t value;
 if(m.word(0x003278ac)!=0){if(!calls.Sub_00183358)hp2::not_implemented("0x001A0C50","Joylog replay service absent");value=calls.Sub_00183358(8,2);calls.Sub_00183358(4,4);}
 else {float elapsed=video*m.scalar(0x003256cc);elapsed=elapsed+m.scalar(0x00327cf0);const bool pal=m.word(0x00327cd4)==0;
 elapsed=std::min(m.scalar(pal?0x003256d4:0x003256dc),std::max(elapsed,m.scalar(pal?0x003256d0:0x003256d8)));
 value=static_cast<std::uint32_t>(GetQuantizedRealTimeElapsed(elapsed));const float decoded=GetRealTimeElapsedFromQuantized(m,static_cast<std::int32_t>(value));
 m.scalar(0x00327cf0,elapsed-static_cast<float>(static_cast<std::int32_t>(packed_time(decoded)))*m.scalar(0x003256e0));
 if(!calls.Sub_00183498)hp2::not_implemented("0x001A0C50","Joylog observation service absent");
 calls.Sub_00183498(value,8,2);calls.Sub_00183498(index,4,4);}
 m.word(0x00327ce8,value);m.scalar(0x00327ce4,GetRealTimeElapsedFromQuantized(m,static_cast<std::int32_t>(value)));
}
// Debug001A0D60.
void AdvanceRealTime(port::driving::Memory& m){const float dt=m.scalar(0x00327ce4),sum=m.scalar(0x00327cec)+dt;const auto n=port::driving::cvt_w_s(sum*60.0f);const auto next=m.word(0x00327ce0)+static_cast<std::uint32_t>(n?n:1);m.word(0x00327cdc,next);m.word(0x00327ce0,next);m.scalar(0x00327cec,sum-static_cast<float>(n)*m.scalar(0x003256e4));m.word(0x00327cf4,m.word(0x00327cf4)+1u);m.word(0x00327cd8,m.word(0x00327cd8)+packed_time(dt));}
// Debug001A0DF8.
void ResetWorldTime(port::driving::Memory& m){const auto n=port::driving::cvt_w_s(m.scalar(0x003256e8));m.word(0x00327d04,1);m.word(0x00327cf8,static_cast<std::uint32_t>(n));m.word(0x00327d00,1);m.word(0x00327d08,0);m.word(0x00327d0c,0);m.word(0x00327d10,0);m.scalar(0x00327cfc,static_cast<float>(n)*m.scalar(0x003256ec));}
// Debug001A0E40.
void PrepareWorldTimestep(port::driving::Memory& m,float dt){m.scalar(0x00327d08,dt);m.word(0x00327d14,0);}
// Debug001A0E50.
void AdvanceWorldTime(port::driving::Memory& m){m.word(0x00327d14,1);const float dt=m.scalar(0x00327d08);if(dt==0)return;const float sum=m.scalar(0x00327d0c)+dt;const auto n=port::driving::cvt_w_s(sum*60.0f);const auto next=m.word(0x00327d04)+static_cast<std::uint32_t>(n);m.word(0x00327d00,next);m.word(0x00327d04,next);m.scalar(0x00327d0c,sum-static_cast<float>(n)*m.scalar(0x003256f0));const auto ticks=m.word(0x00327cf8)+packed_time(dt);m.word(0x00327cf8,ticks);m.word(0x00327d08,0);m.word(0x00327d10,m.word(0x00327d10)+1u);m.scalar(0x00327cfc,static_cast<float>(static_cast<std::int32_t>(ticks))*m.scalar(0x003256f4));}
}

namespace hp2::debug {
// Debug001A1548; TASK-0099 E-0099-W34. Original floor/half-up/ceil order.
float Timer::GetSecondsRounded(const ::port::driving::Memory& m,float iota)const{float value=(static_cast<float>(PackedTime)*m.scalar(0x3256f8))/iota;float whole=static_cast<float>(::port::driving::cvt_w_s(value));if(value<whole)whole=whole-1;if(0.5f<=value-whole)whole=whole+1;value=whole*iota*4000;whole=static_cast<float>(::port::driving::cvt_w_s(value));if(whole<value)whole=whole+1;return whole*m.scalar(0x3256f8);}
// Debug001A1600; E-0099-W34.
void Timer::RoundTime(const ::port::driving::Memory& m,float iota){const float seconds=GetSecondsRounded(m,iota);PackedTime=::port::driving::cvt_w_s(seconds*4000+0.5f);}
}

namespace hp2::debug {
// Debug001A14C8; E-0099-W47. Source packed subtraction and sentinel handling.
int Timer::CountDown(float seconds){if(PackedTime!=0&&PackedTime!=0x7fffffff){PackedTime=static_cast<std::int32_t>(static_cast<std::uint32_t>(PackedTime)-static_cast<std::uint32_t>(::port::driving::cvt_w_s(seconds*4000.f)));if(PackedTime<0)PackedTime=0;}return PackedTime!=0&&PackedTime!=0x7fffffff;}
}

namespace hp2::debug {
// TASK-0099 E-0099-W72. Original fixed-width insertion leaves terminators and
// adjacent template punctuation intact; no snprintf normalization is applied.
void IntToString2(::port::driving::Memory& m,std::uint32_t destination,int value,int digits,bool leading){if(digits<=0)return;for(int i=digits-1;i>=0;--i){const int digit=value%10;const char c=!leading&&digit==0&&value<=9?' ':char(digit+'0');m.byte(destination+std::uint32_t(i),std::uint8_t(c));value/=10;}}
void PrintToString(::port::driving::Memory& m,std::uint32_t out,int flags,int hours,int minutes,int seconds,int hundredths){
 const int language=m.signed_word(0x313898);const bool quote=language==1||language==3,colon=language==2||language==4||language==5;
 const auto copy=[&](std::uint32_t address){for(unsigned i=0;i<64;++i){const auto c=m.byte(address+i);m.byte(out+i,c);if(!c)return;}throw std::out_of_range("Original Timer template has no terminator");};
 if(!hours){if(!minutes){if(!seconds){copy(quote?0x2ad958:colon?0x2ad948:0x2ad938);IntToString2(m,out+6,hundredths,2,true);if(flags&1)m.byte(out,'-');else if(flags&2)m.byte(out,'+');return;}copy(quote?0x2ad928:colon?0x2ad918:0x2ad908);IntToString2(m,out+3,seconds,2,false);}
  else{copy(quote?0x2ad8f8:colon?0x2ad8e8:0x2ad8d8);IntToString2(m,out,minutes,2,false);IntToString2(m,out+3,seconds,2,true);}IntToString2(m,out+6,hundredths,2,true);
 }else{copy(quote?0x2ad8c8:0x2ad8b8);IntToString2(m,out,hours,2,false);IntToString2(m,out+4,minutes,2,true);IntToString2(m,out+7,seconds,2,true);}
 if(flags&1)m.byte(out,'-');else if(flags&2)m.byte(out,'+');
}
void Timer::GetHoursMinsSeconds(int& hours,int& minutes,int& seconds,int& thousandths)const{const int packed=PackedTime,whole=packed/4000;hours=whole/3600;minutes=(whole%3600)/60;seconds=(whole%3600)%60;thousandths=(packed%4000)/4;}
void Timer::PrintToString(::port::driving::Memory& m,std::uint32_t out,int flags){if(!PackedTime||PackedTime==0x7fffffff){hp2::debug::PrintToString(m,out,flags,0,0,0,0);return;}const auto original=PackedTime;const bool negative=original<0;if(negative)PackedTime=std::int32_t(0u-std::uint32_t(original));int hours,minutes,seconds,thousandths;GetHoursMinsSeconds(hours,minutes,seconds,thousandths);PackedTime=original;hp2::debug::PrintToString(m,out,negative?flags|1:flags,hours,minutes,seconds,thousandths/10);}
}

#include "game_core/indep/inc/Replay.hpp"
namespace hp2::debug {
// Debug001A0F20, E-0099-W76. Actual original global fields/order; elapsed is
// VerifyData only and does not silently join ordinary captured state.
void WorldTimeDoSnapshot(::port::driving::Memory& m,std::uint32_t snapshot){ReplaySnapshot s(m,snapshot);s.BeginNamedSection(0x2ad8a8);s.MoveData(0x327cf8,4);s.MoveData(0x327cfc,4);for(auto p:{0x327d10u,0x327d04u,0x327d00u})s.MoveData(p,4,4,1);s.VerifyData(0x327d08,4);s.MoveData(0x327d0c,4);s.EndNamedSection();}
}

#include "game_core/port/race/joylog.hpp"
#include <cmath>
namespace hp2::debug {
// Debug001A0C50, E-0099-W85; original indep/src/timer.cpp. Fresh MIPS proves
// floating MAX.S/MIN.S, not the decompiler's integer casts. Equal operands
// select fs (Sony EE v6 pp363/365, existing CPU-only authorization).
void PrepareRealTimestep(::port::driving::Memory& m,float video,
                         const ::port::race::MonitorSyscall& monitor){
 const auto loop=m.word(0x327cf4)&15u;
 std::uint32_t quantized;
 if(m.word(0x3278ac)){
  quantized=Joylog::GetData(m,8,2,monitor);
  Joylog::GetData(m,4,4,monitor);
 }else{
  float elapsed=video*m.scalar(0x3256cc);
  elapsed=elapsed+m.scalar(0x327cf0);
  const bool pal=m.word(0x327cd4)==0;
  const float minimum=m.scalar(pal?0x3256d0:0x3256d8);
  const float maximum=m.scalar(pal?0x3256d4:0x3256dc);
  if(!std::isfinite(elapsed)||!std::isfinite(minimum)||!std::isfinite(maximum))
   hp2::not_implemented("0x001A0CBC","exceptional EE real-timestep clamp operands");
  const float lower=elapsed>=minimum?elapsed:minimum;
  elapsed=maximum<=lower?maximum:lower;
  quantized=std::uint32_t(hp2::recovered::GetQuantizedRealTimeElapsed(elapsed));
  const float decoded=hp2::recovered::GetRealTimeElapsedFromQuantized(m,std::int32_t(quantized));
  float packed=decoded*4000.f;
  packed=packed+0.5f;
  const auto ticks=::port::driving::cvt_w_s(packed);
  m.scalar(0x327cf0,elapsed-float(ticks)*m.scalar(0x3256e0));
  Joylog::AddData(m,std::int32_t(quantized),8,2,monitor);
  Joylog::AddData(m,std::int32_t(loop),4,4,monitor);
 }
 m.word(0x327ce8,quantized);
 m.scalar(0x327ce4,hp2::recovered::GetRealTimeElapsedFromQuantized(m,std::int32_t(quantized)));
}
}
