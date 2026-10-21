// Port-side native asset and audio-service binding; E-0083-A01.
// Debug NFSHP2.ELF SHA256815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
#include "game_core/src/bMath.hpp"
#include "collision_audio.hpp"
#include "collision_sound_state.hpp"
#include "game_core/port/driving/projections.hpp"
#include "game_core/port/not_implemented.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
namespace port::driving {
namespace {
std::uint32_t word(const std::vector<std::uint8_t>& b,std::size_t p){if(p>b.size()||b.size()-p<4)throw std::runtime_error("Truncated InGame sound word");return b[p]|std::uint32_t(b[p+1])<<8|std::uint32_t(b[p+2])<<16|std::uint32_t(b[p+3])<<24;}
std::uint16_t half(const std::vector<std::uint8_t>& b,std::size_t p){if(p>b.size()||b.size()-p<2)throw std::runtime_error("Truncated InGame sound halfword");return b[p]|std::uint16_t(b[p+1])<<8;}
std::string name(const std::vector<std::uint8_t>& b,std::size_t p){if(p>b.size()||b.size()-p<32)throw std::runtime_error("Truncated InGame sound name");const auto end=std::find(b.begin()+p,b.begin()+p+32,0);if(end==b.begin()+p+32)throw std::runtime_error("Unterminated InGame sound name");return {b.begin()+p,end};}
void big(std::vector<std::uint8_t>& b,std::size_t p,std::uint32_t v){for(unsigned i=0;i<4;++i)b[p+i]=static_cast<std::uint8_t>(v>>(24-i*8));}
VehicleAudioSample decode(const std::vector<std::uint8_t>& h,const std::vector<std::uint8_t>& b,std::uint32_t i,std::uint32_t count){
 const auto at=12+i*52;VehicleAudioSample out;out.name=name(h,at);out.patch_volume=half(h,at+36);
 if((h.at(at+33)>>4)!=0)hp2::not_implemented("0x00260BE0","InGame collision multichannel patch");
 const auto begin=word(h,at+40),end=i+1<count?word(h,at+92):static_cast<std::uint32_t>(b.size());
 if(begin>=end||end>b.size()||begin%16||(end-begin)%16)throw std::runtime_error("Invalid InGame collision ADPCM extent");
 std::uint32_t size=0;bool terminal=false,start=false;
 for(auto p=begin;p<end;p+=16){const auto flags=b[p+1];if(flags&4){start=true;out.loop_begin=(p-begin)/16*28;}size=p-begin+16;if(flags&1){terminal=true;out.loop=(flags&2)!=0;out.loop_end=size/16*28;break;}}
 if(!terminal)throw std::runtime_error("InGame collision ADPCM lacks terminal flag");
 if(out.loop&&!start)hp2::not_implemented("0x00263260","Collision ADPCM loop without repeat-start");
 if(out.loop&&out.loop_begin>=out.loop_end)throw std::runtime_error("Invalid InGame collision loop");
 std::vector<std::uint8_t> vag(48+size);big(vag,0,0x56414770);big(vag,12,size);big(vag,16,half(h,at+34));std::copy_n(b.begin()+begin,size,vag.begin()+48);out.pcm=hp2::port::audio::decode_vag(vag);out.pcm.nominal_gain=float(out.patch_volume)/16383.f;return out;
}
}
// LoadSampleTable001E0D68 + GetInGameSample001AB900; original names/bank hash.
std::vector<VehicleAudioSample> load_ingame_sound_samples(const AudioAssetReader& read,const std::vector<std::string>& names){
 const auto h=read("__Unknown/FDFBE4AB"),b=read("__Unknown/FDFBE4A5");const auto count=word(h,8);
 if(h.size()<12||!count||count>(h.size()-12)/52)throw std::runtime_error("Invalid InGame sound bank count");
 std::vector<VehicleAudioSample> out;
 for(const auto& wanted:names){bool found=false;for(unsigned i=0;i<count;++i)if(name(h,12+i*52)==wanted){out.push_back(decode(h,b,i,count));found=true;break;}if(!found)throw std::runtime_error("Original InGame sample missing: "+wanted);}
 return out;
}
CollisionSoundBank load_collision_sound_bank(const AudioAssetReader& read){
 const std::vector<std::string> names{"CrashMetalSmall01","CrashMetalSmall02","CrashMetalSmall03","CrashMetalMedium01","CrashMetalMedium02","CrashMetalMedium03","CrashMetalHard01","CrashMetalHard02","CrashMetalHard03","CrashWood01","CrashWood02","CrashWood03","CrashFoilage01","CrashFoilage02","CrashFoilage03","CrashMetalScrape","CrashWoodScrape","CrashGlass01"};
 const auto samples=load_ingame_sound_samples(read,names);CollisionSoundBank out;std::copy(samples.begin(),samples.end(),out.samples.begin());return out;
}
// GetCollisionSoundType001E0C18; selection is intentionally not randomized.
std::uint32_t CollisionSoundSelector::select(const Memory& m,int terrain,float impulse){
 if(!std::isfinite(impulse))throw std::invalid_argument("Nonfinite collision impulse");
 unsigned library;if(terrain>14&&terrain<17)library=3;else{const auto level=cvt_w_s((impulse*m.scalar(0x325b78))*3.f);library=static_cast<unsigned>(std::clamp(level,0,2));}
 indices_[library]=(indices_[library]+1)%3;return library*3+indices_[library];
}
// CalcCollisionVolume001E11E8; preserve the strict35 gate and original operation order.
float collision_sound_volume(const Memory& m,float impulse,float distance){
 if(!std::isfinite(impulse)||!std::isfinite(distance)||distance<0)throw std::invalid_argument("Invalid collision sound geometry");
 if(!(distance<35.f))return 0;
 float volume=(impulse*m.scalar(0x325b80))*m.scalar(0x325b84);if(volume>1)volume=1;
 float attenuation=35.f/distance;if(attenuation>1)attenuation=1;return attenuation*volume;
}
bool collision_sound_ready(const Memory& m,std::uint32_t ticks){return m.scalar(0x325ba8)<=static_cast<float>(ticks)*m.scalar(0x325ba4);}
// CreateSample001E1838; allocation/persistent collision ownership is an explicit caller service.
std::optional<CollisionSoundRequest> collision_sound_request(CollisionSoundSelector& selector,Memory& m,int terrain,float impulse,float distance,std::uint32_t age){
 if(!collision_sound_ready(m,age))return {};
 const auto sample=selector.select(m,terrain,impulse);const auto volume=collision_sound_volume(m,impulse,distance);
 if(!(m.scalar(0x325bac)<volume))return {};
 if(!m.word(0x326f40))return CollisionSoundRequest{sample,0,0,false};
 const auto random=hp2::recovered::bRandom(m,0x327eec,m.scalar(0x325bb0));
 return CollisionSoundRequest{sample,volume*m.scalar(0x325bb4),(random+0.5f)-1.f,true};
}
namespace {
// CalcCollisionScrapeVolume001E10C8; initial contact distance is intentional.
float scrape_volume(const Memory& m,float impulse,float speed,float initial_distance){
 if(!std::isfinite(speed))throw std::invalid_argument("Nonfinite collision scrape speed");
 float volume=(collision_sound_volume(m,impulse,initial_distance)*speed)*m.scalar(0x325b7c);
 if(volume>1.f)volume=1.f;
 return volume;
}
}
// AddImpulse00219400 min-speed gate, PlayScrape001E1D80 and StartScraping001E1288.
std::optional<CollisionScrapeRequest> collision_scrape_start(Memory& m,int terrain,
 float impulse,float speed,float initial_distance,std::uint32_t age){
 if(terrain<0||terrain>=33||!std::isfinite(speed))throw std::invalid_argument("Invalid collision scrape terrain/speed");
 if(!(m.scalar(0x3102e8+terrain*0x90+0x88)<speed)||!m.word(0x326f40))return {};
 if(!(m.scalar(0x325b8c)<=static_cast<float>(age)*m.scalar(0x325b88)))return {};
 unsigned sample;if(terrain>14&&terrain<17)sample=16;else if(terrain==17)sample=15;else return {};
 const auto volume=scrape_volume(m,impulse,speed,initial_distance)*m.scalar(0x325b90);
 const auto pitch=hp2::recovered::bRandom(m,0x327eec,m.scalar(0x325b94))+m.scalar(0x325b98);
 return CollisionScrapeRequest{sample,volume,pitch};
}
// Sound3d::ComputeVolume00203E20 envelope with the original200 sound sphere.
float collision_scrape_update_volume(const Memory& m,float impulse,float speed,float initial_distance,float current_distance){
 if(!std::isfinite(current_distance)||current_distance<0)throw std::invalid_argument("Invalid collision scrape camera distance");
 float envelope=0;
 if(!(200.f<current_distance)){const float ratio=current_distance*(1.f/200.f);envelope=1.f-std::sqrt((current_distance+current_distance)*(1.f/200.f)-ratio*ratio);}
 return (envelope*scrape_volume(m,impulse,speed,initial_distance))*m.scalar(0x325b9c);
}
// ReleaseSample001E1970 requests Fade(0,50); fade interval constant3262D4.
float collision_scrape_fade_adjust(const Memory& m,float volume){return (0.f-volume)/(50.f*m.scalar(0x3262d4));}
float collision_scrape_fade_step(float volume,float adjust){const auto next=volume+adjust;return next<0.f?0.f:next;}
bool collision_scrape_fade_finished(const Memory& m,float volume){return volume<=m.scalar(0x325ba0);}
}
