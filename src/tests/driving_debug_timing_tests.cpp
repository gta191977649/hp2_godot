#include "game_core/indep/src/timer.hpp"
#include "game_core/port/driving/projections.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace {void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}}
int main(){
 port::driving::Memory m;m.map(0x00324000,0x4000);
 // E-0044-P05: exact Debug constant words read through Ghidra3256C8..3256F4.
 const std::array<std::uint32_t,12> words{{0x3991a2b4,0x3f82233a,0x3ca3d70a,0x3d75c28f,0x3c888889,0x3d4cccce,0x3983126f,0x3c888889,0x457a0800,0x3983126f,0x3c888889,0x3983126f}};
 for(unsigned i=0;i<words.size();++i)m.word(0x003256c8+4*i,words[i]);
 port::driving::native::ScheduleServices calls;std::vector<std::array<std::uint32_t,3>> observed;
 calls.Sub_00183498=[&](auto v,auto width,auto stream){observed.push_back({{v,width,stream}});};
 m.word(0x00327cd4,1);hp2::recovered::PrepareRealTimestep(m,0,calls);
 check(observed==std::vector<std::array<std::uint32_t,3>>{{{60,8,2}},{{0,4,4}}},"Debug NTSC preparation uses exact quantization and Joylog channels");
 check(m.word(0x00327ce8)==60&&m.word(0x00327ce4)==0x3c888889,"Debug decoded real delta");
 hp2::recovered::ResetWorldTime(m);check(m.word(0x00327cf8)==4000&&m.scalar(0x00327cfc)==1&&m.word(0x00327d10)==0,"Debug world timer initial state");
 hp2::recovered::PrepareWorldTimestep(m,m.scalar(0x00327ce4));check(m.word(0x00327d14)==0,"Prepare clears pending flag");
 hp2::recovered::AdvanceWorldTime(m);check(m.word(0x00327cf8)==4067&&m.word(0x00327d08)==0&&m.word(0x00327d10)==1&&m.word(0x00327d14)==1,"Debug clock advances packed ticks and clears elapsed");
 hp2::recovered::AdvanceWorldTime(m);check(m.word(0x00327cf8)==4067&&m.word(0x00327d10)==1,"Zero delta does not advance loop");
 m.word(0x00327cd4,0);m.word(0x00327cf0,0);hp2::recovered::PrepareRealTimestep(m,0,calls);check(m.word(0x00327ce8)==72,"Debug PAL mode enum0 uses distinct minimum");
 m.word(0x003278ac,1);unsigned reads=0;calls.Sub_00183358=[&](auto width,auto stream){++reads;return width==8&&stream==2?120u:15u;};observed.clear();hp2::recovered::PrepareRealTimestep(m,999,calls);
 check(reads==2&&observed.empty()&&m.word(0x00327ce8)==120,"Debug Joylog replay bypasses elapsed conversion and observation");
 m.word(0x00327ce4,0);m.word(0x00327cec,0);m.word(0x00327ce0,7);hp2::recovered::AdvanceRealTime(m);check(m.word(0x00327cdc)==8&&m.word(0x00327ce0)==8,"Real clock minimum one frame preserved");
 check(hp2::recovered::GetQuantizedRealTimeElapsed(1)==3600,"Real quantization scale differs from packed timer scale");
 std::cout<<"DEBUG_TIMING_OK: canonical entry fixtures, original comparison deferred\n";
}
