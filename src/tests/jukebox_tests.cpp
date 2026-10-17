#include "game_core/bSound/include/bJukebox.h"
#include "platform/ps2_asset_root.hpp"
#include <cstdio>
#include <stdexcept>
#include <fstream>
using namespace hp2::port::audio;
namespace {void require(bool c,const char* s){if(!c)throw std::runtime_error(s);}}
int main(int argc,char** argv) {
    require(Sub_001AF050(4,true) && !Sub_001AF050(4,false) && !Sub_001AF050(2,true),"frontend filter differs");
    std::size_t cursor=0;std::vector<bool> enabled={false,true,false,true};
    require(Sub_0027ECA8(enabled,cursor)==1 && Sub_0027ECA8(enabled,cursor)==3 && Sub_0027ECA8(enabled,cursor)==1,"authored wrap/skip differs");
    const auto prior=cursor;require(!Sub_0027ECA8({false,false},cursor) && cursor==prior,"disabled selection mutates cursor");
    std::uint32_t seed=24;require(bRandom(23,seed)==1 && seed!=24,"RNG return/state differs");
    seed=0xffffffffu;require(bRandom(23,seed)==static_cast<int>(0xffffffffu%23u),"RNG must use DIVU for high-bit seeds");
    const auto root=hp2::platform::resolve_zzdata_root(".");
    if(!root){std::puts("PASS selector; SKIPPED original JBC (root not configured)");return 0;}
    const auto archive=JukeboxCartridge::load_from_zzdata(*root);
    require(archive.tunes().size()==23,"observed JBC tune count differs");
    require(archive.tunes()[1].type==4 && archive.tunes()[0].type==2,"original tune type parser differs");
    FrontendMusic scheduler(archive.tunes());
    require(scheduler.enter(124)==5,"fresh frontend must start preferred tune5");
    require(!scheduler.enter(125),"screen change must preserve existing tune");
    require(scheduler.finished(8000)==7,"natural tune end must advance to next enabled song");
    const auto pcm=archive.decode(argc==3 ? 5 : 1);
    std::printf("JBC PCM rate=%u channels=%u samples=%zu\n",pcm.sample_rate,pcm.channels,pcm.samples.size());
    std::fflush(stdout);
    require(pcm.channels==2 && pcm.sample_rate==32000 && pcm.samples.size()>1000000,"original JBC PCM decode differs");
    require(pcm.nominal_gain==0.45f,"category5 volume differs");
    if(argc==3 && std::string(argv[1])=="--dump-reference-pcm") {
        std::ofstream out(argv[2],std::ios::binary);
        for(const auto sample:pcm.samples) {
            const auto bits=static_cast<std::uint16_t>(sample);
            out.put(static_cast<char>(bits&255));out.put(static_cast<char>(bits>>8));
        }
        if(!out) throw std::runtime_error("PCM differential output failed");
    }
    std::printf("PASS selector + original JBC23 tunes; stereo PCM %zu samples\n",pcm.samples.size());
}
