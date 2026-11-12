// TASK-0099 E-0099-W40. PC hardware adapter for recovered EE command packets.
// Native PCM choices are explicitly port-side; see docs/DECISIONS.md.
#include "pcm_command_device.hpp"
#include "car_sound_resources.hpp"
#include "game_core/indep/src/sound.hpp"
#include "game_core/port/not_implemented.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace port::race {
namespace {
struct Command {unsigned code=0;std::vector<std::uint16_t> words;};
unsigned channel_index(std::uint16_t word){const auto channel=unsigned(word);if(channel>=48)throw std::out_of_range("PC sound command channel outside48-voice device");return channel;}
// Port PCM mapping of the raw15-bit signed volume register value. Recovery
// records retain the unmodified raw value; this is not a new game gain curve.
float pcm_gain(std::uint16_t raw){const auto bits=raw&0x7fff;const auto signed_value=(bits&0x4000)?int(bits)-0x8000:int(bits);return float(signed_value)/16383.0f;}
}
unsigned PcmCommandDevice::consume(driving::Memory& memory,CarSoundResources& source){
 const auto extent=memory.word(0x3291b8);const auto count=memory.half(0x31a140);
 if(extent>4095)throw std::out_of_range("PC sound command batch exceeds original senddata span");
 std::vector<Command> commands;commands.reserve(count);unsigned cursor=1;
 for(unsigned i=0;i<count;++i){if(cursor>extent||extent-cursor+1<2)throw std::out_of_range("Truncated original sound command header");Command cmd;cmd.code=memory.half(0x31a140+cursor*2);const auto words=memory.half(0x31a140+(cursor+1)*2);cursor+=2;if(cursor>extent+1||words>extent+1-cursor)throw std::out_of_range("Truncated original sound command payload");for(unsigned j=0;j<words;++j)cmd.words.push_back(memory.half(0x31a140+(cursor+j)*2));cursor+=words;commands.push_back(std::move(cmd));}
 if(cursor!=extent+1)throw std::out_of_range("Original sound batch count/extent disagree");
 std::lock_guard<std::mutex> lock(mutex_);auto next=channels_;
 for(const auto& cmd:commands){const auto& w=cmd.words;switch(cmd.code){
  case 0:{if(w.size()!=8)throw std::out_of_range("Original PlaySFX command requires eight words");if(w[7])hp2::not_implemented("0x002630C8","PC device interpretation of a nonzero original IOP playback offset is not recovered");if(w[6])hp2::not_implemented("0x002630C8","PC device interpretation of a nonzero original IOP repeat-address override is not recovered");auto& voice=next[channel_index(w[1])];voice.patch=source.decode_physical_patch(w[0]);voice.patch_id=w[0];voice.left=w[2];voice.right=w[3];voice.pitch=w[4];voice.attack_release=w[5];voice.loop=w[6];voice.offset=w[7];voice.cursor=0;voice.active=true;break;}
  case 2:if(w.size()!=3)throw std::out_of_range("Original channel volume command requires three words");next[channel_index(w[0])].left=w[1];next[channel_index(w[0])].right=w[2];next[channel_index(w[0])].smooth_duration=0;break;
  case 3:if(w.size()!=2)throw std::out_of_range("Original channel pitch command requires two words");next[channel_index(w[0])].pitch=w[1];break;
  case 0xcb:if(w.size()!=4)throw std::out_of_range("Original smooth volume command requires four words");next[channel_index(w[0])].left=w[1];next[channel_index(w[0])].right=w[2];next[channel_index(w[0])].smooth_duration=w[3];break;
  case 4:if(w.size()!=1)throw std::out_of_range("Original StopSound command requires one word");next[channel_index(w[0])].active=false;break;
  case 0x18:case 0x19:if(w.size()!=1)throw std::out_of_range("Original channel-effect command requires one word");next[channel_index(w[0])].effect_enabled=cmd.code==0x18;break;
  case 9:if(w.size()!=2)throw std::out_of_range("Original status command requires two words");if(w[0]!=0xffff||w[1]!=0)hp2::not_implemented("0x00263820","PC device does not implement original asynchronous stream/bank status requests");break;
  default:hp2::not_implemented("0x00262540","original sound command has no concrete PC device consumer");
 }}
 channels_=std::move(next);consumed_commands_+=commands.size();return extent;
}
std::vector<float> PcmCommandDevice::render(std::size_t frames,unsigned output_rate){
 if(output_rate<8000||output_rate>192000||frames>0x100000)throw std::invalid_argument("PC PCM output clock or frame span outside bounded device domain");
 std::vector<float> output(frames*2);std::lock_guard<std::mutex> lock(mutex_);
 for(auto& voice:channels_){if(!voice.active)continue;if(!voice.patch||voice.patch->samples.empty())throw std::logic_error("Active PCM voice lacks its decoded physical mono source");const auto& patch=*voice.patch;const auto& samples=patch.samples;const double step=(48000.0*double(voice.pitch)/4096.0)/double(output_rate);const auto left=pcm_gain(voice.left),right=pcm_gain(voice.right);
  for(std::size_t frame=0;frame<frames;++frame){const auto finish=patch.repeat?patch.loop_end:std::uint32_t(samples.size());if(voice.cursor>=finish){if(patch.repeat)voice.cursor=patch.loop_begin+std::fmod(voice.cursor-patch.loop_begin,double(patch.loop_end-patch.loop_begin));else{voice.active=false;break;}}
   const auto index=std::size_t(voice.cursor);const auto next=index+1<finish?index+1:(patch.repeat?patch.loop_begin:index);const float fraction=float(voice.cursor-double(index));const float sample=(float(samples[index])+(float(samples[next])-float(samples[index]))*fraction)/32768.0f;output[frame*2]+=sample*left;output[frame*2+1]+=sample*right;voice.cursor+=step;
  }
 }
 for(auto& sample:output)sample=std::clamp(sample,-1.0f,1.0f);
 return output;
}
std::array<PcmChannelState,48> PcmCommandDevice::snapshot()const{std::lock_guard<std::mutex> lock(mutex_);return channels_;}
std::uint64_t PcmCommandDevice::consumed_commands()const{std::lock_guard<std::mutex> lock(mutex_);return consumed_commands_;}
void PcmCommandDevice::publish_resident_status(driving::Memory& memory)const{std::lock_guard<std::mutex> lock(mutex_);for(unsigned i=0;i<48;++i){const std::uint8_t key=channels_[i].active?3:0;const auto previous=memory.byte(0x319b00+i);memory.byte(0x319b00+i,!key&&previous==3?4:key);}}
}

#include "game_core/src/bDriverPS2.hpp"
#include <limits>
namespace port::race {
namespace {
std::uint32_t source_word(const std::vector<std::uint8_t>& b,std::size_t at){if(at>b.size()||b.size()-at<4)throw std::out_of_range("Truncated authored sound source word");return b[at]|(std::uint32_t(b[at+1])<<8)|(std::uint32_t(b[at+2])<<16)|(std::uint32_t(b[at+3])<<24);}
std::uint16_t source_half(const std::vector<std::uint8_t>& b,std::size_t at){if(at>b.size()||b.size()-at<2)throw std::out_of_range("Truncated authored sound source halfword");return std::uint16_t(b[at]|(unsigned(b[at+1])<<8));}
void vag_word(std::vector<std::uint8_t>& b,std::size_t at,std::uint32_t value){for(unsigned i=0;i<4;++i)b[at+i]=std::uint8_t(value>>(24-i*8));}
}
// Port source binding. Every physical MSH row has its own original SFX ID and
// offset, including individual stereo planes; no name-based mono stand-in.
std::shared_ptr<const DecodedSoundPatch> CarSoundResources::decode_physical_patch(std::uint16_t id){
 if(!software_voices_)hp2::not_implemented("0x0025FC58","PC device requires initialized actual SDK source tables");
 const auto total=memory_.half(0x3290e8);if(id>=total)throw std::out_of_range("PC device patch ID outside retained authored table");if(decoded_patches_.size()!=total)decoded_patches_.resize(total);if(decoded_patches_[id])return decoded_patches_[id];
 std::size_t first=0;for(const auto& bank:banks_){const auto count=source_word(bank.metadata,8);if(id>=first+count){first+=count;continue;}const auto physical=std::uint32_t(id-first);const auto row=std::size_t(12)+physical*52;const auto begin=source_word(bank.metadata,row+40);std::size_t end=bank.data.size();
  // Use the next greater authored offset, not physical row order: stereo plane
  // IDs are consumed in reverse. The ADPCM terminal flag is the actual end.
  for(unsigned i=0;i<count;++i){const auto offset=source_word(bank.metadata,12+std::size_t(i)*52+40);if(offset>begin)end=std::min(end,std::size_t(offset));}
  if(begin>=end||end>bank.data.size()||begin%16||(end-begin)%16)throw std::out_of_range("Invalid authored physical ADPCM patch extent");
  auto decoded=std::make_shared<DecodedSoundPatch>();const auto name_end=std::find(bank.metadata.begin()+row,bank.metadata.begin()+row+32,0);if(name_end==bank.metadata.begin()+row+32)throw std::out_of_range("Unterminated authored physical patch name");decoded->name={bank.metadata.begin()+row,name_end};bool terminal=false,repeat_start=false;std::size_t size=0;
  for(std::size_t at=begin;at<end;at+=16){const auto flags=bank.data[at+1];if(flags&4){repeat_start=true;decoded->loop_begin=std::uint32_t((at-begin)/16*28);}size=at-begin+16;if(flags&1){terminal=true;decoded->repeat=(flags&2)!=0;decoded->loop_end=std::uint32_t(size/16*28);break;}}
  if(!terminal)throw std::out_of_range("Authored ADPCM patch has no terminal block flag");
  if(decoded->repeat&&!repeat_start)hp2::not_implemented("0x00263260","PC ADPCM repeat without an authored repeat-start flag is not recovered");
  if(decoded->repeat&&decoded->loop_begin>=decoded->loop_end)throw std::out_of_range("Invalid authored PCM repeat span");
  if(size/16*28>std::numeric_limits<std::uint32_t>::max())throw std::out_of_range("Authored PCM patch exceeds bounded frame index");
  std::vector<std::uint8_t> vag(48+size);vag_word(vag,0,0x56414770);vag_word(vag,12,std::uint32_t(size));vag_word(vag,16,source_half(bank.metadata,row+34));std::copy_n(bank.data.begin()+begin,size,vag.begin()+48);auto pcm=hp2::port::audio::decode_vag(vag);if(pcm.channels!=1)throw std::logic_error("Physical sound patch decoder did not produce mono PCM");decoded->samples=std::move(pcm.samples);decoded->sample_rate=pcm.sample_rate;decoded_patches_[id]=decoded;return decoded;
 }
 throw std::logic_error("Actual SDK patch interval has no retained authored bank");
}
// Port hardware binding of FlushIOPCommand00264CF0. The recovered request,
// batch header/reset and ticker writes are retained; native consumption replaces
// RPC transfer. There is no fabricated full IOP/CD/stream status reply.
int CarSoundResources::flush_audio(PcmCommandDevice& device,std::uint32_t ticker,int transfer_flag){
 if(!software_voices_)hp2::not_implemented("0x00264CF0","actual SDK software voice state is not initialized");
 auto& m=memory_;
 if(!m.byte(0x329220))hp2::debug::SOUND_GetStatus(m,-1,0);
 m.half(0x31a140,std::uint16_t(m.word(0x329268)));
 const auto extent=device.consume(m,*this);m.byte(0x329220,0);m.word(0x3291b8,0);m.word(0x329268,0);m.word(0x329248,ticker);device.publish_resident_status(m);
 // The PC device consumes synchronously for both original RPC scheduling flags;
 // this is a native transport policy, not a claim of original IOP latency.
 (void)transfer_flag;return int(extent);
}
}
