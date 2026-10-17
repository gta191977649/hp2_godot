// Original source path, recovered verbatim from the authorized ELF: `..\indep\src\sound.cpp`
// (string at 0x002B62E8, SLUS_203.62 sha256 7335b8ed...facc2; TASK-0020, E-0020-N01).
// This file sits at that path. The path is original; what the file holds is a
// partial port-side reconstruction, not the original translation unit.
#include "sound.hpp"
#include "game_core/port/not_implemented.hpp"
#include <algorithm>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace hp2::port::audio {
namespace {
std::uint32_t word(const std::vector<std::uint8_t>& b, std::size_t o, bool big = false) {
    if (o > b.size() || b.size() - o < 4) throw std::runtime_error("truncated sound asset word");
    std::uint32_t value = 0;
    for (unsigned i = 0; i < 4; ++i) value |= std::uint32_t(b[o+i]) << (8*(big ? 3-i : i));
    return value;
}
void put_big(std::vector<std::uint8_t>& b,std::size_t o,std::uint32_t value) {
    for(unsigned i=0;i<4;++i) b[o+i]=static_cast<std::uint8_t>(value>>(8*(3-i)));
}
int floor_div(int value, int divisor) {
    return value >= 0 ? value/divisor : -((-value + divisor-1)/divisor);
}
}

PcmSound decode_vag(const std::vector<std::uint8_t>& b, std::size_t interleave_bytes) {
    if (b.size() < 48 || word(b,0,true) != 0x56414770u)
        throw std::runtime_error("sound payload is not a 48-byte-header VAGp");
    const auto size = word(b,12,true);
    PcmSound result;
    result.sample_rate = word(b,16,true);
    if (word(b,36,true)==0x56414778u) result.channels=word(b,44,true);
    if (result.channels!=1 && result.channels!=2)
        hp2::not_implemented("0x00266FB0", "unobserved VAGx channel layout");
    if (!result.sample_rate || size % 16 || size > b.size()-48)
        throw std::runtime_error("invalid VAGp rate/ADPCM payload size");
    // Port codec format constants, not recovered HP2 constants. Rounded/clipped history
    // matches the authorized map_tools_ps2 reference (E-0016-A06), including short tails.
    constexpr int coefficients[5][2] = {{0,0},{60,0},{115,-52},{98,-55},{122,-60}};
    if (!interleave_bytes || interleave_bytes%16 || size % (16*result.channels))
        throw std::runtime_error("incomplete VAG channel block group");
    int previous[2] = {}, older[2] = {};
    result.samples.reserve(size/16*28);
    const auto group_size=interleave_bytes*result.channels;
    for(std::size_t group=0;group<size;group+=group_size) {
        const auto remaining=size-group;
        const auto channel_bytes=remaining>=group_size ? interleave_bytes : remaining/result.channels;
        if(!channel_bytes || channel_bytes%16) throw std::runtime_error("partial VAG interleave is not aligned");
        const auto frame_count=channel_bytes/16*28,output_start=result.samples.size();
        result.samples.resize(output_start+frame_count*result.channels);
        for(unsigned channel=0;channel<result.channels;++channel) {
            std::size_t sample_index=0;
            for(std::size_t offset=0;offset<channel_bytes;offset+=16) {
                const auto o=48+group+channel*channel_bytes+offset;
                const auto predictor=b[o]>>4,shift=b[o]&15;
                if(predictor>=5 || shift>12) throw std::runtime_error("invalid PSX ADPCM predictor/shift");
                for(unsigned n=0;n<28;++n) {
                    int nibble=(b[o+2+n/2]>>(4*(n%2)))&15;
                    if(nibble>=8)nibble-=16;
                    const auto prediction=floor_div(previous[channel]*coefficients[predictor][0]+
                                                     older[channel]*coefficients[predictor][1]+32,64);
                    const auto sample=std::clamp(floor_div(nibble*4096,1<<shift)+prediction,-32768,32767);
                    result.samples[output_start+sample_index*result.channels+channel]=static_cast<std::int16_t>(sample);
                    ++sample_index;older[channel]=previous[channel];previous[channel]=sample;
                }
            }
        }
    }
    return result;
}

std::uint32_t Sub_00273850(std::uint32_t base_rate) {
    return static_cast<std::uint32_t>(static_cast<float>(base_rate)*(4140.0f/4096.0f));
}
std::uint16_t SOUND_HzToSPU(std::uint32_t frequency) {
    return static_cast<std::uint16_t>((frequency<<12)/48000u);
}
float neutral_playback_pitch_scale(std::uint32_t decoded_rate) {
    if(!decoded_rate) throw std::runtime_error("zero decoded sample rate");
    return (48000.0f*SOUND_HzToSPU(Sub_00273850(decoded_rate))/4096.0f)/decoded_rate;
}

GuestSampleBank GuestSampleBank::from_bytes(std::vector<std::uint8_t> bytes) {
    GuestSampleBank bank;
    bank.bytes_ = std::move(bytes);
    const auto& b = bank.bytes_;
    if (b.size()<28 || word(b,0)!=0x14200 || word(b,12)!=0x14201 ||
        std::string(reinterpret_cast<const char*>(b.data()+20),5)!="Guest")
        throw std::runtime_error("not the observed Guest sound bank");
    for (std::size_t o=0; o<b.size();) {
        const auto tag=word(b,o), size=word(b,o+4);
        if (size>b.size()-o-8) throw std::runtime_error("truncated Guest sound bank chunk");
        const auto end=o+8+size;
        if (tag==0x80014103u) {
            if (size<16 || word(b,o+8)!=1) hp2::not_implemented("0x00266FB0", "E-0016-A02: sound sample chunk variant");
            const auto name_size=word(b,o+12);
            if (!name_size || name_size>end-o-24) throw std::runtime_error("invalid sample name size");
            const auto name_start=o+16, data_chunk=name_start+name_size;
            if (word(b,data_chunk)!=0x14204) hp2::not_implemented("0x00266FB0", "E-0016-A02: non-VAG sample chunk");
            const auto payload_size=word(b,data_chunk+4);
            if (payload_size>end-data_chunk-8) throw std::runtime_error("truncated sample payload");
            const auto* name=reinterpret_cast<const char*>(b.data()+name_start);
            std::size_t count=0;
            while (count<name_size && name[count]) ++count;
            bank.entries_.push_back({std::string(name,count),data_chunk+8,payload_size});
        }
        o=end;
    }
    if (bank.entries_.empty()) throw std::runtime_error("Guest sound bank has no samples");
    return bank;
}

GuestSampleBank GuestSampleBank::load_from_zzdata(const std::string& root) {
    // Observed original hashed archive member, E-0016-A02. No proprietary data is bundled.
    std::ifstream stream(root+"/__Unknown/E2F0CC8A",std::ios::binary);
    if (!stream) throw std::runtime_error("missing original Guest sample bank __Unknown/E2F0CC8A");
    auto bank=from_bytes(std::vector<std::uint8_t>(std::istreambuf_iterator<char>(stream),{}));
    std::ifstream descriptors(root+"/__Unknown/84CBF303",std::ios::binary);
    if (!descriptors) throw std::runtime_error("missing original common sound descriptors 84CBF303");
    const std::vector<std::uint8_t> d(std::istreambuf_iterator<char>(descriptors),{});
    if (d.size()<12 || word(d,8)> (d.size()-12)/52)
        throw std::runtime_error("truncated common sound descriptors");
    for (unsigned i=0;i<word(d,8);++i) {
        const auto o=12+i*52;
        std::size_t n=0;while(n<32 && d[o+n]) ++n;
        const std::string name(reinterpret_cast<const char*>(d.data()+o),n);
        const auto gain=std::uint16_t(d[o+36]) | (std::uint16_t(d[o+37])<<8);
        bank.gains_.push_back({name,static_cast<float>(gain)*0.00006103888f});
    }
    // E-0016-A03: the actual common PS2 sample bank is planar stereo ADPCM.
    // Adapt it in memory to our codec input, retaining its leading hardware zero blocks.
    std::ifstream data_stream(root+"/__Unknown/84CBF2FD",std::ios::binary);
    if (!data_stream) throw std::runtime_error("missing original common sound data 84CBF2FD");
    const std::vector<std::uint8_t> raw(std::istreambuf_iterator<char>(data_stream),{});
    const auto count=word(d,8);
    if(count%2) throw std::runtime_error("unpaired common stereo descriptor");
    for(unsigned i=0;i<count;i+=2) {
        const auto o=12+i*52;
        const auto left=word(d,o+40),right=word(d,o+52+40);
        const auto end=i+2<count ? word(d,o+104+40) : raw.size();
        if(left>right || right>end || end>raw.size() || right-left!=end-right || (right-left)%16)
            throw std::runtime_error("invalid common stereo sample offsets");
        const auto rate=std::uint16_t(d[o+34])|(std::uint16_t(d[o+35])<<8);
        std::vector<std::uint8_t> vag(48+(end-left),0);
        put_big(vag,0,0x56414770);put_big(vag,12,static_cast<std::uint32_t>(end-left));
        put_big(vag,16,rate);put_big(vag,36,0x56414778);put_big(vag,44,2);
        for(std::size_t n=0;n<right-left;n+=16) {
            std::copy_n(raw.begin()+left+n,16,vag.begin()+48+n*2);
            std::copy_n(raw.begin()+right+n,16,vag.begin()+64+n*2);
        }
        std::size_t n=0;while(n<32 && d[o+n]) ++n;
        const auto start=bank.bytes_.size();
        bank.bytes_.insert(bank.bytes_.end(),vag.begin(),vag.end());
        bank.entries_.insert(bank.entries_.begin(),{std::string(reinterpret_cast<const char*>(d.data()+o),n),start,vag.size()});
    }
    return bank;
}
std::vector<std::string> GuestSampleBank::names() const {
    std::vector<std::string> result;
    for (const auto& entry:entries_) result.push_back(entry.name);
    return result;
}
PcmSound GuestSampleBank::decode(const std::string& event_name) const {
    for (const auto& entry:entries_) if (entry.name==event_name) {
        auto result=decode_vag(std::vector<std::uint8_t>(bytes_.begin()+entry.offset,
                                                      bytes_.begin()+entry.offset+entry.size));
        for (const auto& gain:gains_) if(gain.first==event_name) {result.nominal_gain=gain.second;break;}
        return result;
    }
    hp2::not_implemented("0x002719B8", "E-0016-A01: requested bSound event absent in Guest asset bank");
}
} // namespace hp2::port::audio
