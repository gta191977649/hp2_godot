#include "game_core/movieplayer/movieplayer.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <algorithm>

namespace {
using Bytes=std::vector<std::uint8_t>;
void word(Bytes& b,std::uint32_t v){for(unsigned i=0;i<4;++i)b.push_back(static_cast<std::uint8_t>(v>>(i*8)));}
Bytes chunk(const char* id,Bytes data){Bytes b(id,id+4);word(b,static_cast<std::uint32_t>(data.size()));b.insert(b.end(),data.begin(),data.end());if(data.size()&1)b.push_back(0);return b;}
void append(Bytes& b,const Bytes& tail){b.insert(b.end(),tail.begin(),tail.end());}
void setword(Bytes& b,unsigned pos,std::uint32_t v){for(unsigned i=0;i<4;++i)b[pos+i]=static_cast<std::uint8_t>(v>>(i*8));}
Bytes fixture(){
    Bytes sh(56);std::copy_n("vidsmv30",8,sh.begin());setword(sh,20,1);setword(sh,24,30);
    Bytes sf(40);setword(sf,0,40);setword(sf,4,16);setword(sf,8,16);sf[12]=1;sf[14]=24;std::copy_n("MV30",4,sf.begin()+16);
    Bytes stream{'s','t','r','l'};append(stream,chunk("strh",sh));append(stream,chunk("strf",sf));
    Bytes hdrl{'h','d','r','l'};append(hdrl,chunk("LIST",stream));
    Bytes movi{'m','o','v','i'};append(movi,chunk("00dc",{1,2,3}));append(movi,chunk("00dc",{4,5}));
    Bytes avi{'A','V','I',' '};append(avi,chunk("LIST",hdrl));append(avi,chunk("LIST",movi));return chunk("RIFF",avi);
}
}

int main(int argc,char** argv) {
    try {
        const auto check=[](bool v){if(!v)throw std::runtime_error("movie/startup mismatch");};
        using port::media::StartupAction;
        hp2::frontend::Type_0016F000 s;
        auto result=s.Construct_0016F000(false,false,0,100);
        check(result.action==StartupAction::PlayMovie&&std::string(result.filename)=="MOVIES/INTRO/EALOGO_EN.AVI");
        check(!result.start_music);
        result=s.Activate_0016EEF8(101);
        check(result.action==StartupAction::None&&s.Field_0x08==0&&s.Field_0x0c==2&&!result.start_music); // first EA logo cannot be skipped
        check(s.Update_0016F1F0(false,200).action==StartupAction::None);
        result=s.Update_0016F1F0(true,201);
        check(result.action==StartupAction::PlayMovie&&std::string(result.filename)=="MOVIES/INTRO/NFSINTRO_EN.AVI");
        result=s.Update_0016F1F0(true,202);
        check(result.action==StartupAction::ShowTitle&&result.start_music);
        check(s.Activate_0016EEF8(16202).action==StartupAction::None); // exactly four seconds
        check(s.Activate_0016EEF8(16203).action==StartupAction::RequestMainMenu);
        s.Construct_0016F000(false,false,1,300);
        s.Update_0016F1F0(true,300);
        result=s.Activate_0016EEF8(301);
        check(result.action==StartupAction::ShowTitle&&!result.start_music);
        check(s.Field_0x08==1); // second intro skip jumps to title; no music call in this handler
        s.Update_0016F1F0(false,302);
        check(s.Activate_0016EEF8(16303).action==StartupAction::RequestMainMenu);
        s.Construct_0016F000(false,true,2,400);
        check(s.Activate_0016EEF8(400).action==StartupAction::None);
        result=s.Update_0016F1F0(true,401);
        check(result.action==StartupAction::ShowTitle&&!result.start_music);
        check(s.Activate_0016EEF8(20000).action==StartupAction::None); // demo blocks main transition
        result=s.Construct_0016F000(true,false,0,500);
        check(result.action==StartupAction::ShowTitle&&result.start_music);
        result=s.Construct_0016F000(true,true,0,500);
        check(result.action==StartupAction::ShowTitle&&result.start_music); // ctor bypass calls music regardless demo flag
        s.Field_0x14=0;check(!s.Sub_0016F4F8(200000));
        s.Field_0x14=0x7fffffff;check(!s.Sub_0016F4F8(0x800fffff));
        s.Field_0x14=0xfffffff0;check(s.Sub_0016F4F8(16000)); // native 32-bit wrap
        for(int language=0;language<6;++language) {
            check(std::string(hp2::frontend::Sub_0016EE90(language,1,false))==(language==1?"MOVIES/INTRO/NFSINTRO_FR.AVI":"MOVIES/INTRO/NFSINTRO_EN.AVI"));
        }
        bool rejected=false;try{port::media::read_avi({0,1,2});}catch(const std::runtime_error&){rejected=true;}check(rejected);
        Bytes oriented={1,2,3,4,5,6,7,8, 9,10,11,12,13,14,15,16, 17,18,19,20,21,22,23,24};
        const auto original=oriented;
        port::media::orient_rgba_for_ps2(oriented,2,3);
        check(oriented==Bytes({17,18,19,20,21,22,23,24, 9,10,11,12,13,14,15,16, 1,2,3,4,5,6,7,8}));
        port::media::orient_rgba_for_ps2(oriented,2,3);check(oriented==original);
        rejected=false;try{port::media::orient_rgba_for_ps2(oriented,2,4);}catch(const std::runtime_error&){rejected=true;}check(rejected);
        auto synthetic=fixture();auto parsed=port::media::read_avi(synthetic);
        check(parsed.width==16&&parsed.height==16&&parsed.rate==30&&parsed.scale==1);
        check(parsed.video_frames.size()==2&&parsed.video_frames[0]==Bytes({1,2,3})&&parsed.video_frames[1]==Bytes({4,5}));
        synthetic.pop_back();rejected=false;try{port::media::read_avi(synthetic);}catch(const std::runtime_error&){rejected=true;}check(rejected);
        // Optional private-asset check: no asset bytes or captures enter the repository.
        for(int a=1;a<argc;++a) {
            std::ifstream f(argv[a],std::ios::binary);if(!f)throw std::runtime_error("movie fixture cannot open");
            std::vector<std::uint8_t> b(std::istreambuf_iterator<char>(f),{});
            const auto movie=port::media::read_avi(b);
            port::media::MovieDecoder decoder(movie.width,movie.height);
            std::uint64_t sum=0;
            for(std::size_t frame=0;frame<std::min<std::size_t>(movie.video_frames.size(),10);++frame) {
                const auto rgba=decoder.decode_frame(movie.video_frames[frame]);check(rgba.size()==std::size_t(movie.width)*movie.height*4);
                for(std::size_t i=3;i<rgba.size();i+=4)check(rgba[i]==255);
                for(auto byte:rgba)sum+=byte;
            }
            std::cout<<"MOVIE_DECODE_OK "<<movie.width<<'x'<<movie.height<<" frames="<<movie.video_frames.size()<<" rate="<<movie.rate<<'/'<<movie.scale<<" pcm="<<movie.audio_data.size()<<" checksum="<<sum<<'\n';
        }
        std::cout<<"MOVIE_STARTUP_OK\n";
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
