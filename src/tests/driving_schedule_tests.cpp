#include "game_core/indep/src/timer.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/port/driving/projections.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <vector>
using port::driving::Memory;
using namespace port::driving::native;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int run(){
    Memory memory;memory.map(0x00300000,0x40000);memory.map(0x002e7e00,24);
    memory.word(0x003084ec,0x11223344);memory.word(0x003084fc,0x55667788);
    hp2::recovered::Sub_0019B358(memory);
    check(memory.word(0x003084ec)==0x11223344&&memory.word(0x003084fc)==0x55667788,"Static initializer preserves unmentioned fourth lanes");
    check(memory.word(0x00308518)==0x00308500&&memory.word(0x0030851c)==6&&memory.scalar(0x00308520)==0&&memory.scalar(0x00308524)==10&&memory.scalar(0x00308528)==0.5f,"Body first table has six original samples across0..10");
    check(memory.word(0x00308550)==0x00308530&&memory.word(0x00308554)==8&&memory.scalar(0x00308558)==0&&memory.scalar(0x0030855c)==1&&memory.scalar(0x00308560)==7,"Body second table has eight original samples across0..1");
    memory.word(0x00324e34,0x3ecccccd);memory.word(0x00324e38,0x3db2b8c4);
    hp2::recovered::Sub_00139510(memory);
    check(memory.word(0x003270c0)==0x002e7e00&&memory.word(0x003270c4)==3,"Dynamic irregular table header is initialized rather than read as zero ELF data");
    check(memory.scalar(0x002e7e00)==0&&memory.scalar(0x002e7e04)==0&&memory.word(0x002e7e08)==0x3ecccccd&&memory.scalar(0x002e7e0c)==0&&memory.scalar(0x002e7e10)==2&&memory.scalar(0x002e7e14)==20,"Irregular point stores are(0,0),(global,0),(2,20)");
    check(memory.word(0x00327080)==0x3db2b8c4,"Initializer publishes the second raw global");
    // E-0044-P05 Debug3256C8..3256F4 exact constant words; captured through Ghidra.
    const std::array<std::uint32_t,12> words{{0x3991a2b4,0x3f82233a,0x3ca3d70a,0x3d75c28f,0x3c888889,0x3d4cccce,
        0x3983126f,0x3c888889,0x457a0800,0x3983126f,0x3c888889,0x3983126f}};
    for(unsigned i=0;i<words.size();++i)memory.word(0x003256c8+4*i,words[i]);
    memory.word(0x00327cd4,1); // Debug VIDEO_MODE:1=MODE_NTSC;0=MODE_PAL.
    std::vector<std::array<std::uint32_t,3>> observations;ScheduleServices services;
    services.Sub_00183498=[&](auto value,auto width,auto stream){observations.push_back({{value,width,stream}});};
    hp2::recovered::PrepareRealTimestep(memory,0,services);
    check(observations==std::vector<std::array<std::uint32_t,3>>{{{60,8,2}},{{0,4,4}}},"Default mode clamps then records quantized60, independent of guessed frame rate");
    check(memory.word(0x00327ce8)==60&&memory.word(0x00327ce4)==0x3c888889,"Quantized delta is60 times exact original reciprocal3600");
    Memory one_second=memory;hp2::recovered::ResetWorldTime(one_second);hp2::recovered::PrepareWorldTimestep(one_second,1);hp2::recovered::AdvanceWorldTime(one_second);
    check(hp2::recovered::GetQuantizedRealTimeElapsed(1)==3600&&one_second.word(0x00327cf8)==8000,"Distinct real quantization and packed timing conversion scales are preserved");
    hp2::recovered::ResetWorldTime(memory);check(memory.word(0x00327cf8)==4000&&memory.scalar(0x00327cfc)==1&&memory.word(0x00327d08)==0,"Clock constructor truncates original4000.5 constant");
    hp2::recovered::PrepareWorldTimestep(memory,memory.scalar(0x00327ce4));check(memory.word(0x00327d14)==0,"Delta publication clears end-frame marker");
    hp2::recovered::AdvanceWorldTime(memory);check(memory.word(0x00327cf8)==4067&&memory.word(0x00327d08)==0&&memory.word(0x00327d10)==1&&memory.word(0x00327d14)==1,"End-frame advances4000Hz integer counter, clears delta and sets marker");
    const auto old=memory.word(0x00327cf8);hp2::recovered::AdvanceWorldTime(memory);check(memory.word(0x00327cf8)==old&&memory.word(0x00327d10)==1,"Zero delta leaves counters unchanged");
    observations.clear();memory.word(0x00327cd4,0);memory.word(0x00327cf0,0);hp2::recovered::PrepareRealTimestep(memory,0,services);
    check(memory.word(0x00327ce8)==72,"Other original mode minimum quantizes to72 rather than60");
    memory.word(0x003278ac,1);unsigned reads=0;services.Sub_00183358=[&](auto width,auto stream){++reads;return width==8&&stream==2?120u:15u;};
    observations.clear();hp2::recovered::PrepareRealTimestep(memory,999,services);
    check(reads==2&&observations.empty()&&memory.word(0x00327ce8)==120,"Replay branch uses both original reads and no elapsed clamp");
    memory.word(0x00327ce4,0);memory.word(0x00327cec,0);memory.word(0x00327ce0,7);hp2::recovered::AdvanceRealTime(memory);
    check(memory.word(0x00327cdc)==8&&memory.word(0x00327ce0)==8,"Render clock advances at least one tick for zero delta");
    std::cout<<"Driving static initialization and scheduling fixtures passed; original equivalence remains unverified.\n";return 0;
}
}
int main(){try{return run();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
