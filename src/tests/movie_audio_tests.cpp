#include "game_core/port/movieplayer/movie_audio.hpp"
#include "game_core/indep/src/movieplayer/movieplayer.hpp"
#include "platform/ps2_asset_root.hpp"
#include <fstream>
#include <cstdio>
#include <stdexcept>
#include <algorithm>
using namespace hp2::port::audio;
namespace {void require(bool c,const char* s){if(!c)throw std::runtime_error(s);}}
int main(int argc,char** argv) {
    std::vector<std::uint8_t> raw(65536+32,0);
    for(std::size_t b=0;b<raw.size()/16;++b) {
        raw[b*16]=12;const auto nibble=b<2048?1:b<4096?2:b==4096?3:4;
        std::fill_n(raw.begin()+b*16+2,14,static_cast<std::uint8_t>(nibble*17));
    }
    auto audio=decode_movie_audio(raw);
    require(audio.pcm.channels==2 && audio.pcm.sample_rate==48000,"movie host stream format differs");
    require(audio.pitch_scale==3762.0f/4096.0f,"original SPU pitch differs");
    require(audio.pcm.samples[0]==1 && audio.pcm.samples[1]==2 &&
            audio.pcm.samples[114688]==3 && audio.pcm.samples[114689]==4,"movie channel planes/tail differ");
    bool rejected=false;try{decode_movie_audio({0});}catch(const std::runtime_error&){rejected=true;}
    require(rejected,"invalid movie ADPCM accepted");
    const auto root=hp2::platform::resolve_zzdata_root(".");
    if(!root){std::puts("PASS synthetic movie codec; SKIPPED original AVI (assetroot missing)");return 0;}
    std::ifstream f(*root+"/MOVIES/INTRO/EALOGO_EN.AVI",std::ios::binary);
    std::vector<std::uint8_t> bytes(std::istreambuf_iterator<char>(f),{});
    const auto avi=port::media::read_avi(bytes);
    audio=decode_movie_audio(avi.audio_data);
    require(avi.audio_data.size()==327680 && audio.pcm.samples.size()==573440,"original EALOGO ADPCM byte/frame count differs");
    require(std::any_of(audio.pcm.samples.begin(),audio.pcm.samples.end(),[](auto x){return x!=0;}),"original movie decoded silence");
    if(argc==3 && std::string(argv[1])=="--dump-reference-pcm") {
        std::ofstream out(argv[2],std::ios::binary);
        for(auto s:audio.pcm.samples){const auto u=static_cast<std::uint16_t>(s);out.put(static_cast<char>(u&255));out.put(static_cast<char>(u>>8));}
        if(!out)throw std::runtime_error("movie reference output failed");
    }
    std::ifstream nfs_file(*root+"/MOVIES/INTRO/NFSINTRO_EN.AVI",std::ios::binary);
    std::vector<std::uint8_t> nfs_bytes(std::istreambuf_iterator<char>(nfs_file),{});
    const auto nfs=port::media::read_avi(nfs_bytes);
    const auto nfs_audio=decode_movie_audio(nfs.audio_data);
    require(nfs.audio_data.size()==4128768 && nfs_audio.pcm.samples.size()==7225344,
            "original NFS intro audio byte/frame count differs");
    require(nfs_audio.pcm.channels==2 && nfs_audio.pitch_scale==audio.pitch_scale,
            "movie carrier headers must not change hardware channel/pitch parameters");
    std::printf("PASS original movie ADPCM stereo %zu frames; exacthardware %.7f Hz\n",audio.pcm.samples.size()/2,48000.0*audio.pitch_scale);
    std::printf("NFS original movie stereo %zu frames; padded stream duration %.7f seconds\n",
                nfs_audio.pcm.samples.size()/2,(nfs_audio.pcm.samples.size()/2)/(48000.0*nfs_audio.pitch_scale));
}
