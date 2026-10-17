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
    Memory memory;memory.map(0x00300000,0x40000);memory.map(0x002f0570,24);
    memory.word(0x00310dcc,0x11223344);memory.word(0x00310ddc,0x55667788);
    Sub_0019DA30(memory);
    check(memory.word(0x00310dcc)==0x11223344&&memory.word(0x00310ddc)==0x55667788,"Static initializer preserves unmentioned fourth lanes");
    check(memory.word(0x00310df8)==0x00310de0&&memory.word(0x00310dfc)==6&&memory.scalar(0x00310e00)==0&&memory.scalar(0x00310e04)==10&&memory.scalar(0x00310e08)==0.5f,"Body first table has six original samples across0..10");
    check(memory.word(0x00310e30)==0x00310e10&&memory.word(0x00310e34)==8&&memory.scalar(0x00310e38)==0&&memory.scalar(0x00310e3c)==1&&memory.scalar(0x00310e40)==7,"Body second table has eight original samples across0..1");
    memory.word(0x0032dd48,0x3ecccccd);memory.word(0x0032dd4c,0x3db2b8c4);
    Sub_001395B0(memory);
    check(memory.word(0x00330060)==0x002f0570&&memory.word(0x00330064)==3,"Dynamic irregular table header is initialized rather than read as zero ELF data");
    check(memory.scalar(0x002f0570)==0&&memory.scalar(0x002f0574)==0&&memory.word(0x002f0578)==0x3ecccccd&&memory.scalar(0x002f057c)==0&&memory.scalar(0x002f0580)==2&&memory.scalar(0x002f0584)==20,"Irregular point stores are(0,0),(global,0),(2,20)");
    check(memory.word(0x00330020)==0x3db2b8c4,"Initializer publishes the second raw global");
    // Original ELF words0032E5F4..0032E620; preserve exact representation.
    const std::array<std::uint32_t,12> words{{0x3991a2b4,0x3f82233a,0x3ca3d70a,0x3d75c28f,0x3c888889,0x3d4cccce,
        0x3983126f,0x3c888889,0x457a0800,0x3983126f,0x3c888889,0x3983126f}};
    for(unsigned i=0;i<words.size();++i)memory.word(0x0032e5f4+4*i,words[i]);
    memory.word(0x0032e624,0x3983126f);
    memory.word(0x00330c54,1); // ELF initial mode chooses alternate min/max.
    std::vector<std::array<std::uint32_t,3>> observations;ScheduleServices services;
    services.Sub_00183498=[&](auto value,auto width,auto stream){observations.push_back({{value,width,stream}});};
    Sub_001A33F0(memory,0,services);
    check(observations==std::vector<std::array<std::uint32_t,3>>{{{60,8,2}},{{0,4,4}}},"Default mode clamps then records quantized60, independent of guessed frame rate");
    check(memory.word(0x00330c68)==60&&memory.word(0x00330c64)==0x3c888889,"Quantized delta is60 times exact original reciprocal3600");
    check(GetQuantizedRealTimeElapsed(1)==3600&&Sub_00136DD0(1)==4000,"Distinct timing conversion scales are preserved");
    Sub_001A3598(memory);check(memory.word(0x00330c78)==4000&&memory.scalar(0x00330c7c)==1&&memory.word(0x00330c88)==0,"Clock constructor truncates original4000.5 constant");
    Sub_001A35E0(memory,memory.scalar(0x00330c64));check(memory.word(0x00330c94)==0,"Delta publication clears end-frame marker");
    Sub_001A35F0(memory);check(memory.word(0x00330c78)==4067&&memory.word(0x00330c88)==0&&memory.word(0x00330c90)==1&&memory.word(0x00330c94)==1,"End-frame advances4000Hz integer counter, clears delta and sets marker");
    const auto old=memory.word(0x00330c78);Sub_001A35F0(memory);check(memory.word(0x00330c78)==old&&memory.word(0x00330c90)==1,"Zero delta leaves counters unchanged");
    observations.clear();memory.word(0x00330c54,0);memory.word(0x00330c70,0);Sub_001A33F0(memory,0,services);
    check(memory.word(0x00330c68)==72,"Other original mode minimum quantizes to72 rather than60");
    memory.word(0x00330834,1);unsigned reads=0;services.Sub_00183358=[&](auto width,auto stream){++reads;return width==8&&stream==2?120u:15u;};
    observations.clear();Sub_001A33F0(memory,999,services);
    check(reads==2&&observations.empty()&&memory.word(0x00330c68)==120,"Replay branch uses both original reads and no elapsed clamp");
    memory.word(0x00330c64,0);memory.word(0x00330c6c,0);memory.word(0x00330c60,7);Sub_001A3500(memory);
    check(memory.word(0x00330c5c)==8&&memory.word(0x00330c60)==8,"Render clock advances at least one tick for zero delta");
    std::cout<<"Driving static initialization and scheduling fixtures passed; original equivalence remains unverified.\n";return 0;
}
}
int main(){try{return run();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
