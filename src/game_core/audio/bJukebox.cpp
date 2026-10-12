#include "bJukebox.hpp"
#include "game_core/not_implemented.hpp"
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace hp2::port::audio {
namespace {
std::uint32_t word(const std::vector<std::uint8_t>& b,std::size_t o) {
    if(o>b.size() || b.size()-o<4) throw std::runtime_error("truncated jukebox metadata");
    return std::uint32_t(b[o])|(std::uint32_t(b[o+1])<<8)|(std::uint32_t(b[o+2])<<16)|(std::uint32_t(b[o+3])<<24);
}
std::string text(const std::vector<std::uint8_t>& b,std::size_t o,std::size_t max) {
    if(o>b.size() || max>b.size()-o) throw std::runtime_error("truncated jukebox name");
    std::size_t n=0;while(n<max && b[o+n])++n;
    return std::string(reinterpret_cast<const char*>(b.data()+o),n);
}
}
JukeboxCartridge JukeboxCartridge::load_from_zzdata(const std::string& root) {
    JukeboxCartridge result;
    result.path_=root+"/SOUND/JUKEBOXM.JBC"; // Exact shipped filename,001AE608.
    std::ifstream f(result.path_,std::ios::binary);
    if(!f) throw std::runtime_error("missing original SOUND/JUKEBOXM.JBC");
    std::vector<std::uint8_t> h(32);f.read(reinterpret_cast<char*>(h.data()),32);
    if(!f || word(h,0)!=16 || word(h,4)!=2 || word(h,8)!=32000)
        HP2_NOT_IMPLEMENTED("0x0027DAC8","unobserved jukebox cartridge format");
    const auto category_count=word(h,12),metadata_size=word(h,16);
    if(metadata_size>1024*1024) throw std::runtime_error("oversized jukebox metadata");
    std::vector<std::uint8_t> metadata(metadata_size);
    f.read(reinterpret_cast<char*>(metadata.data()),metadata_size);
    if(!f) throw std::runtime_error("truncated jukebox metadata table");
    std::size_t o=0;
    for(unsigned c=0;c<category_count;++c) {
        const auto count=word(metadata,o);
        const auto category=text(metadata,o+4,60);o+=64;
        for(unsigned t=0;t<count;++t) {
            MusicTune tune;
            tune.category=category;tune.offset=word(metadata,o);tune.size=word(metadata,o+4);
            tune.sync_size=word(metadata,o+8);tune.name=text(metadata,o+12,84);
            // Original001AED90 splits on '-' without collapsing empty fields.
            std::size_t start=0;int field=0;
            while(field<4) {
                const auto p=tune.name.find('-',start);
                if(p==std::string::npos) HP2_NOT_IMPLEMENTED("0x001AEFD0","jukebox name missing type field");
                start=p+1;++field;
            }
            try {tune.type=std::stoi(tune.name.substr(start));}
            catch(const std::exception&) {throw std::runtime_error("invalid jukebox type field");}
            result.tunes_.push_back(std::move(tune));o+=96;
        }
    }
    return result;
}
PcmSound JukeboxCartridge::decode(std::size_t index) const {
    const auto& tune=tunes_.at(index);
    if(tune.sync_size) HP2_NOT_IMPLEMENTED("0x0027CA00","jukebox synchronous prelude not implemented");
    std::ifstream f(path_,std::ios::binary);f.seekg(tune.offset);
    std::vector<std::uint8_t> bytes(tune.size);
    f.read(reinterpret_cast<char*>(bytes.data()),tune.size);
    if(!f) throw std::runtime_error("truncated jukebox original tune payload");
    auto pcm=decode_vag(bytes,0x8000);
    pcm.nominal_gain=0.45f; // ELF category5 default bits0x3ee66666 at0032E758.
    return pcm;
}
bool Sub_001AF050(int type,bool genre_enabled) {
    if(type<0 || type>5) HP2_NOT_IMPLEMENTED("0x001AF050","jukebox song type outside recovered matrix");
    constexpr bool frontend[6]={false,false,false,false,true,true};
    return frontend[type] && genre_enabled;
}
std::optional<std::size_t> Sub_0027ECA8(const std::vector<bool>& enabled,std::size_t& cursor) {
    if(enabled.empty()) return std::nullopt;
    for(std::size_t step=1;step<=enabled.size();++step) {
        const auto index=(cursor+step)%enabled.size();
        if(enabled[index]) {cursor=index;return index;}
    }
    return std::nullopt;
}
int Sub_0025A3D0(int count,std::uint32_t& seed) {
    if(count<=0) throw std::runtime_error("RNG modulo count must be positive");
    const auto old=seed,a=old^0x1d872b41u,b=a^(a>>5);
    seed=b^a^(b<<27);
    // Listing0025A3E4 is DIVU: decompiler's signed modulo is counterevidence.
    return static_cast<int>(old%static_cast<std::uint32_t>(count));
}
FrontendMusic::FrontendMusic(const std::vector<MusicTune>& tunes) {
    for(const auto& tune:tunes) enabled_.push_back(Sub_001AF050(tune.type,true));
}
std::optional<std::size_t> FrontendMusic::enter(std::uint32_t frame_counter) {
    if(playing_) return std::nullopt;
    if(first_frontend_) {
        first_frontend_=false;
        if(enabled_.size()>5 && enabled_[5]) {cursor_=5;playing_=true;return cursor_;}
    }
    if(first_random_ && !enabled_.empty()) {
        first_random_=false;
        auto seed=frame_counter;
        const auto count=Sub_0025A3D0(static_cast<int>(enabled_.size()),seed);
        for(int i=0;i<count;++i) Sub_0027ECA8(enabled_,cursor_);
    }
    const auto result=Sub_0027ECA8(enabled_,cursor_);
    playing_=result.has_value();return result;
}
std::optional<std::size_t> FrontendMusic::finished(std::uint32_t frame_counter) {
    playing_=false;
    // 0027DFF8 auto-mode1 immediately selects next tune; the random one-time fallback
    // only runs on a later explicit001AEAB0 call when no tune is playing.
    (void)frame_counter;
    const auto result=Sub_0027ECA8(enabled_,cursor_);
    playing_=result.has_value();return result;
}
} // namespace hp2::port::audio
