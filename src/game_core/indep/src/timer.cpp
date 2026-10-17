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
