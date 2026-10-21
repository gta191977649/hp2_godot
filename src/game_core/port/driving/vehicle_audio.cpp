// Port-owned original-asset/audio-service binding. E-0082-A01/A02.
// Debug NFSHP2.ELF SHA-256 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
#include "vehicle_audio.hpp"
#include "game_core/port/driving/projections.hpp"
#include "game_core/port/not_implemented.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace port::driving {
namespace {
std::uint32_t word(const std::vector<std::uint8_t>& b,std::size_t p){
    if(p>b.size()||b.size()-p<4)throw std::runtime_error("Truncated original vehicle audio word");
    return b[p]|std::uint32_t(b[p+1])<<8|std::uint32_t(b[p+2])<<16|std::uint32_t(b[p+3])<<24;
}
std::uint16_t half(const std::vector<std::uint8_t>& b,std::size_t p){
    if(p>b.size()||b.size()-p<2)throw std::runtime_error("Truncated original vehicle audio halfword");
    return b[p]|std::uint16_t(b[p+1])<<8;
}
float scalar(const std::vector<std::uint8_t>& b,std::size_t p){auto bits=word(b,p);float value;std::memcpy(&value,&bits,4);return value;}
std::string text(const std::vector<std::uint8_t>& b,std::size_t p,std::size_t size){
    if(p>b.size()||size>b.size()-p)throw std::runtime_error("Truncated original vehicle audio name");
    const auto end=std::find(b.begin()+p,b.begin()+p+size,0);
    if(end==b.begin()+p+size)throw std::runtime_error("Unterminated original vehicle audio name");
    return {b.begin()+p,end};
}
std::string upper(std::string value){for(auto& c:value)c=static_cast<char>(std::toupper(static_cast<unsigned char>(c)));return value;}
std::string hashed_header(const std::string& bank){
    std::uint32_t hash=0xffffffffu;
    for(unsigned char c:upper("SOUND\\"+bank+".MSH"))hash=hash*33u+c;
    std::ostringstream path;path<<"__Unknown/"<<std::uppercase<<std::hex<<std::setw(8)<<std::setfill('0')<<hash;return path.str();
}
void put_big(std::vector<std::uint8_t>& b,std::size_t at,std::uint32_t value){for(unsigned i=0;i<4;++i)b[at+i]=static_cast<std::uint8_t>(value>>(24-8*i));}
VehicleAudioSample decode_patch(const std::vector<std::uint8_t>& header,const std::vector<std::uint8_t>& data,std::uint32_t index,std::uint32_t count){
    const auto row=12u+index*52u;VehicleAudioSample out;out.name=text(header,row,32);out.patch_volume=half(header,row+36);
    // GetChannelNum00260BE0 returns LoopChannel>>4. This binding is mono only.
    if((header.at(row+33)>>4)!=0)hp2::not_implemented("0x00260BE0","Vehicle bank multichannel patch layout");
    const auto begin=word(header,row+40),end=index+1<count?word(header,row+92):static_cast<std::uint32_t>(data.size());
    if(begin>=end||end>data.size()||begin%16||(end-begin)%16)throw std::runtime_error("Invalid original vehicle ADPCM patch extent");
    std::uint32_t size=0;bool terminal=false,loop_start=false;
    for(auto at=begin;at<end;at+=16){
        const auto flags=data[at+1];
        if(flags&4){out.loop_begin=((at-begin)/16)*28;loop_start=true;}
        size=at-begin+16;
        if(flags&1){out.loop=(flags&2)!=0;out.loop_end=(size/16)*28;terminal=true;break;}
    }
    if(!terminal)throw std::runtime_error("Original vehicle ADPCM patch lacks terminal flag");
    if(out.loop&&!loop_start)hp2::not_implemented("0x00263260","Vehicle ADPCM loop without observed repeat-start flag");
    if(out.loop&&out.loop_begin>=out.loop_end)throw std::runtime_error("Invalid original vehicle PCM loop extent");
    std::vector<std::uint8_t> vag(48+size);put_big(vag,0,0x56414770);put_big(vag,12,size);put_big(vag,16,half(header,row+34));
    std::copy_n(data.begin()+begin,size,vag.begin()+48);out.pcm=hp2::port::audio::decode_vag(vag);
    out.pcm.nominal_gain=static_cast<float>(out.patch_volume)/16383.0f;
    return out;
}
}

// Init0020AE48, Load_File00260560, SOUND_PATCH_INFO type/asset bytes; E-0082-A01.
VehicleSoundBank load_vehicle_sound_bank(const AudioAssetReader& read,const std::array<std::uint8_t,0x560>& config){
    const std::vector<std::uint8_t> raw(config.begin(),config.end());VehicleSoundBank out;
    out.basename=text(raw,0x330,32);out.prefix=text(raw,0x356,8);out.horn_pitch=scalar(raw,0x350);
    out.idle_offset=half(raw,0x35e);out.bend_range=static_cast<std::int16_t>(half(raw,0x364));out.bass_add=static_cast<std::int16_t>(half(raw,0x366));
    for(unsigned i=0;i<4;++i)out.timbre[i]=static_cast<std::int8_t>(raw[0x360+i]);
    for(unsigned i=0;i<12;++i){out.detune[i]=static_cast<std::int16_t>(half(raw,0x368+2*i));out.volume[i]=scalar(raw,0x380+4*i);}
    out.redline_offset=static_cast<std::int16_t>(half(raw,0x3b0));out.topgear_offset=static_cast<std::int16_t>(half(raw,0x3b2));
    auto header=read(hashed_header(out.basename)),data=read("SOUND/"+upper(out.basename)+".MSB");
    const auto count=word(header,8);if(count==0||count>(header.size()-std::min<std::size_t>(header.size(),12))/52)throw std::runtime_error("Invalid vehicle MSH patch count");
    std::vector<VehicleAudioSample> samples;for(std::uint32_t i=0;i<count;++i)samples.push_back(decode_patch(header,data,i,count));
    // Exact suffix pointer array00328930, selected by Init0020AE48.
    const std::array<const char*,12> suffix{{"lelo","lemd","lehi","lxlo","lxmd","lxhi","ceid","cemd","cehi","cxid","cxmd","cxhi"}};
    const auto sample=[&](const std::string& name){for(const auto& s:samples)if(upper(s.name)==upper(name))return s;throw std::runtime_error("Original vehicle patch missing: "+name);};
    for(unsigned i=0;i<12;++i)out.engine[i]=sample(out.prefix+suffix[i]);
    out.horn=sample(out.prefix+"Horn");
    out.definitions[0]=read("SOUND/ENGINE.CTB");out.definitions[1]=read("SOUND/ENGINE.LTB");
    for(const auto& def:out.definitions){
        if(def.size()<360||word(def,0)!=0x6c445243u||def[4]!=2||def[5]!=0)throw std::runtime_error("Unsupported original engine definition header");
        for(unsigned i=0;i<8;++i)if(static_cast<std::int8_t>(def[32+i])>=0){
            if(def[32+i]>=12)throw std::runtime_error("Original engine definition sample outside12 patches");
            const auto at=296u+4*i,offset=word(def,at);
            if(offset>def.size()-at||512>def.size()-at-offset)throw std::runtime_error("Original engine definition volume table outside file");
        }
    }
    return out;
}

// bSample::CalcPitch0026BA98, bFloatToInt002578D8, SetVoicePitch0025C7A8; E-0082-A02.
float vehicle_sample_pitch_scale(const Memory& m,std::uint32_t rate,float pitch){
    if(!rate||!std::isfinite(pitch))throw std::invalid_argument("Invalid original vehicle sample pitch input");
    const int range=m.byte(0x3292ec);if(range<2)throw std::runtime_error("Original octave table range unavailable");
    int coarse=static_cast<std::int8_t>(cvt_w_s(pitch));
    if(std::abs(coarse)>=range-1){pitch=std::clamp(pitch,float(1-range),float(range-1));coarse=static_cast<std::int8_t>(cvt_w_s(pitch));}
    float scale;
    if(-2.0f<=pitch&&pitch<=2.0f){const int index=cvt_w_s((pitch+2.0f)*64.0f)+1;scale=static_cast<float>(m.half(0x31eb40+2*index))*0.000244140625f;}
    else{
        const auto index=coarse+range;const float first=m.scalar(0x31eaf8+4*index);
        const float second=m.scalar(0x31eaf8+4*(coarse<1?index-1:index));
        scale=first+second*(pitch-static_cast<float>(coarse));
    }
    const auto hertz=static_cast<std::uint32_t>(cvt_w_s(static_cast<float>(rate)*scale));
    const auto spu=hp2::port::audio::SOUND_HzToSPU(hertz);
    return (48000.0f*static_cast<float>(spu)/4096.0f)/static_cast<float>(rate);
}
// bSample GetScaledVolume0026BEC8/CalcVolume0026C120; category/scale host binding.
float vehicle_sample_gain_scale(const Memory& m,std::uint16_t patch_volume,float volume,float category){
    float scaled=static_cast<float>(patch_volume)*category;scaled=scaled*m.scalar(0x3262c8);
    const auto raw=static_cast<std::uint16_t>(cvt_w_s((volume*scaled)*m.scalar(0x3262cc)));
    return static_cast<float>(raw)/16383.0f;
}
}
