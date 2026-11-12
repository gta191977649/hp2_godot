// Port source-data lifetime, E-0099-W31. No whole bSound device/SPU startup is
// asserted: only the original region/sample lookup data are imported here.
#include "car_sound_resources.hpp"
#include "game_core/src/bSound.hpp"
#include "game_core/src/bRegion.hpp"
#include "game_core/src/bSample.hpp"
#include "game_core/include/bVoice.h"
#include "game_core/src/bDriverPS2.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include "game_core/port/not_implemented.hpp"
#include "fragmented_pointer_view.hpp"
#include "game_core/port/assets/elab_chunks.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <sstream>
#include <iomanip>
namespace port::race {
namespace {
std::vector<std::uint8_t> read_audio_source(const std::filesystem::path& path){std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("Original authored car sound input absent: "+path.string());return {std::istreambuf_iterator<char>(f),{}};}
std::string audio_upper(std::string s){for(auto& c:s)if(std::uint32_t(std::uint8_t(c)-'a')<26)c=char(std::uint8_t(c)&0x5f);return s;}
std::uint32_t audio_word(const std::vector<std::uint8_t>& b,std::size_t p){if(p>b.size()||b.size()-p<4)throw std::out_of_range("Truncated authored sound metadata word");return b[p]|(std::uint32_t(b[p+1])<<8)|(std::uint32_t(b[p+2])<<16)|(std::uint32_t(b[p+3])<<24);}
std::string audio_name(const std::vector<std::uint8_t>& b,std::size_t p){if(p>b.size()||b.size()-p<32)throw std::out_of_range("Truncated authored patch name");const auto end=std::find(b.begin()+p,b.begin()+p+32,0);if(end==b.begin()+p+32)throw std::out_of_range("Unterminated authored patch name");return {b.begin()+p,end};}
std::filesystem::path audio_asset_path(const std::filesystem::path& root,const std::string& name,const char* extension){
 const auto file=audio_upper(name)+extension;const auto direct=root/"SOUND"/file;if(std::filesystem::exists(direct))return direct;
 const auto original="SOUND\\"+file;const auto hash=hp2::frontend::bStringHash(original.c_str());std::ostringstream encoded;encoded<<std::uppercase<<std::hex<<std::setw(8)<<std::setfill('0')<<hash;return root/"__Unknown"/encoded.str();}
}
std::uint32_t CarSoundResources::copy_bytes(const std::vector<std::uint8_t>& bytes){if(bytes.empty()||bytes.size()>0xffffffffu)throw std::out_of_range("Invalid authored sound data span");const auto out=memory_.allocate(std::uint32_t(bytes.size()));std::memcpy(memory_.view(out,bytes.size()).data,bytes.data(),bytes.size());allocations_.push_back(out);return out;}
void CarSoundResources::initialize_vector(std::uint32_t at){memory_.word(at,0);memory_.word(at+4,0);fragmented_count(memory_,at+8,0);fragmented_count(memory_,at+16,0);}
// Original pointer-vector schema/growth:1 initial slot, multiplier2; nodes have
// next/data plus64-bit start/length. Storage helper remains explicitly port-side.
void CarSoundResources::append_pointer(std::uint32_t at,std::uint32_t value){
 auto& m=memory_;const auto size=fragmented_count(m,at+8),capacity=fragmented_count(m,at+16);
 if(size==capacity){const auto next_capacity=std::max<std::uint64_t>(1,capacity*2),extra=next_capacity-capacity;
  if(next_capacity>0x3fffffffu)throw std::overflow_error("Original sound pointer-vector growth exceeds logical extent");
  const auto node=m.allocate(24),data=m.allocate(std::uint32_t(extra)*4);allocations_.push_back(node);allocations_.push_back(data);
  m.word(node,0);m.word(node+4,data);fragmented_count(m,node+8,capacity);fragmented_count(m,node+16,extra);
  const auto tail=m.word(at+4);if(tail)m.word(tail,node);else m.word(at,node);m.word(at+4,node);fragmented_count(m,at+16,next_capacity);
 }
 fragmented_count(m,at+8,size+1);
 for(auto node=m.word(at);node;node=m.word(node)){const auto start=fragmented_count(m,node+8),length=fragmented_count(m,node+16);if(size>=start&&size-start<length){m.word(m.word(node+4)+std::uint32_t(size-start)*4,value);return;}}
 throw std::logic_error("Sound source pointer-vector append has no node");
}
void CarSoundResources::initialize_lookup_state(){
 auto& m=memory_;if(initialized_ || m.word(0x32d770+0x158) || m.word(0x32d770+8)!=0)throw std::logic_error("Sound lookup source state must be initialized before platform startup and existing regions");
 if(!m.word(0x3292e8))hp2::not_implemented("0x0026B468","original bSample pool must be initialized before importing real sound regions");
 // Confirmed lookup constructor fields from bSound0026F768. The sound driver
 // status stays0; no asynchronous device initialization or loaded-SPU claim.
 m.word(0x32d770+0x4c,0);m.word(0x32d770+0x50,0);initialize_vector(0x32d770+0x158);initialize_vector(0x32d770+0x170);
 m.word(0x329320,0x32d770);m.word(0x32d770+4,0);m.word(0x32d770+8,0);
 for(unsigned i=0;i<16;++i)m.scalar(0x32d770+0xc+4*i,1);
 m.byte(0x32d770+0x188,0);m.word(0x327f24,0);initialized_=true;
}
void CarSoundResources::load_region(const std::filesystem::path& root,const std::string& name){
 if(software_voices_)throw std::logic_error("Sound source tables must be loaded before software voices borrow patch IDs");
 if(!initialized_)throw std::logic_error("Sound lookup source owner is not initialized");
 if(name.empty()||name.size()>32)throw std::out_of_range("Original sound region name outside retained field");
 if(hp2::debug::bSound(memory_,0x32d770).Region(name))throw std::logic_error("Duplicate actual sound region binding");
 Bank bank;bank.name=name;bank.metadata=read_audio_source(audio_asset_path(root,name,".MSH"));bank.data=read_audio_source(audio_asset_path(root,name,".MSB"));
 const auto count=audio_word(bank.metadata,8);if(bank.metadata.size()<12 || count>(bank.metadata.size()-12)/52)throw std::out_of_range("Invalid complete authored sound patch table");
 bank.header=copy_bytes(bank.metadata);copy_bytes(bank.data);bank.region=memory_.allocate(64);allocations_.push_back(bank.region);
 std::memcpy(memory_.view(bank.region,name.size()+1).data,name.c_str(),name.size()+1);initialize_vector(bank.region+0x28);
 for(std::uint32_t i=0;i<count;++i){const auto row=12+i*52;audio_name(bank.metadata,row);
  // Original region sample enumeration counts terminal channel-number0 rows;
  // all physical source rows are retained for patch-channel queries.
  if((bank.metadata[row+33]>>4)!=0)continue;
  const auto sample=hp2::debug::bSample::Sub_0026DD98(memory_);hp2::debug::bSample(memory_,sample).Sub_0026B2A0();
  hp2::debug::bSample(memory_,sample).SetName(bank.header+row);hp2::debug::bSample(memory_,sample).SetBankName(bank.region);
  append_pointer(bank.region+0x28,sample);bank.samples.push_back(sample);
 }
 append_pointer(0x32d770+0x158,bank.region);banks_.push_back(std::move(bank));
}
void CarSoundResources::load_engine_definitions(const std::filesystem::path& root){for(unsigned i=0;i<2;++i){definitions_[i]=read_audio_source(root/"SOUND"/(i?"ENGINE.LTB":"ENGINE.CTB"));if(definitions_[i].size()<360 || audio_word(definitions_[i],0)!=0x6c445243u || definitions_[i][4]!=2 || definitions_[i][5]!=0)throw std::out_of_range("Unsupported complete original EngineDef source");}}
std::uint32_t CarSoundResources::in_game_sample(const std::string& name){auto region=memory_.word(0x327f24);if(!region){region=hp2::debug::bSound(memory_,memory_.word(0x329320)).Region("InGame");memory_.word(0x327f24,region);if(!region)return 0;}return hp2::debug::bRegion(memory_,region).Sample(name);}
std::uint32_t CarSoundResources::region(const std::string& name)const{return hp2::debug::bSound(memory_,memory_.word(0x329320)).Region(name);}
std::uint32_t CarSoundResources::sample(const std::string& bank,const std::string& name)const{const auto region=hp2::debug::bSound(memory_,memory_.word(0x329320)).Region(bank);return region?hp2::debug::bRegion(memory_,region).Sample(name):0;}
std::uint16_t CarSoundResources::channels(const std::string& bank,const std::string& name)const{
 // bSnd_GetPatchID0025FC58 scans raw rows backward with case-sensitive
 // bank16/patch32 comparisons. bRegion Sample has a different case-folded scan.
 for(const auto& source:banks_)if(source.name.substr(0,16)==bank.substr(0,16)){const auto count=audio_word(source.metadata,8);for(std::uint32_t i=count;i>0;--i)if(audio_name(source.metadata,12+(i-1)*52).substr(0,32)==name.substr(0,32))return std::uint16_t((source.metadata[12+(i-1)*52+33]>>4)+1);}
 return 0;
}
std::uint32_t CarSoundResources::engine_definition(unsigned i){if(i>=2 || definitions_[i].empty())hp2::not_implemented("0x0020AE48","complete original engine definition source has not been loaded");const auto at=memory_.allocate(std::uint32_t(definitions_[i].size()));std::memcpy(memory_.view(at,definitions_[i].size()).data,definitions_[i].data(),definitions_[i].size());return at;}
void CarSoundResources::initialize_software_voices(){
 auto& m=memory_;if(software_voices_||!source_ready()||m.word(0x3290d4)||m.word(0x3290d8))throw std::logic_error("Software sound lookup owner requires fresh SDK tables and actual source banks");
 if(m.word(0x3291b8)||m.word(0x329268)||m.signed_word(0x329264)!=-1)throw std::logic_error("Cannot initialize software sound over an existing IOP command batch");
 std::size_t patches=0;for(const auto& b:banks_)patches+=audio_word(b.metadata,8);
 if(!patches||patches>0x7fff||banks_.size()>0xffff)throw std::out_of_range("Authored SDK sound table exceeds original signed16 lookup domain");
 std::vector<std::uint8_t> rows(patches*52),regions(banks_.size()*20);std::size_t first=0,index=0;
 const auto half=[&](std::size_t at,std::uint16_t value){regions[at]=std::uint8_t(value);regions[at+1]=std::uint8_t(value>>8);};
 for(const auto& b:banks_){const auto count=audio_word(b.metadata,8);if(!count)throw std::out_of_range("Empty authored bank has no registered SDK patch interval");std::copy_n(b.metadata.begin()+12,std::size_t(count)*52,rows.begin()+first*52);std::copy_n(b.name.begin(),std::min<std::size_t>(16,b.name.size()),regions.begin()+index*20);half(index*20+16,std::uint16_t(first));half(index*20+18,std::uint16_t(first+count-1));first+=count;++index;}
 const auto region_table=copy_bytes(regions),patch_table=copy_bytes(rows);
 // Reviewed raw source lookup projection. Offsets remain authored relative
 // offsets; no SPU upload/relocation or whole bSnd_Initialize is claimed.
 m.word(0x3290d4,region_table);m.word(0x3290d8,patch_table);m.half(0x3290e6,std::uint16_t(banks_.size()));m.half(0x3290e8,std::uint16_t(patches));
 // Exact software prefix of SOUND_InitIOP002622B0. RPC binding remains absent.
 m.word(0x31eac0,0);for(unsigned i=0;i<48;++i){if(m.byte(0x319550+i))throw std::logic_error("Cannot initialize software voices while channels are locked");m.byte(0x319ce0+i,0);m.byte(0x319d70+i,0);m.byte(0x319d10+i,0);m.byte(0x319b00+i,0);m.half(0x319b60+i*2,0);m.half(0x319bc0+i*2,0);m.word(0x319c20+i*4,0);m.byte(0x319b30+i,0xff);hp2::debug::ClearVoice(m,std::uint16_t(i));}
 for(unsigned i=0;i<4;++i){m.half(0x329250+i*2,0x1000);m.half(0x329258+i*2,0x1000);}
 hp2::debug::bSnd_SetStereo(m);hp2::debug::bVoiceMgr::Initialize(m);
 for(const auto& b:banks_)for(const auto sample:b.samples)m.half(sample+0x94,hp2::debug::bSnd_GetPatchID(m,sample+0x58,sample+0x38));
 software_voices_=true;
}
std::string CarSoundResources::load_track_sound_info(const std::vector<std::uint8_t>& bytes,int number){
 if(track_sound_info_ || memory_.word(0x327398))throw std::logic_error("CurrentTrackInfo already has an owner");
 std::size_t consumed=0;const auto chunks=hp2::assets::parse_chunks_top(bytes,&consumed);
 if(consumed!=bytes.size())throw std::out_of_range("Incomplete authored track catalog bundle");
 const auto visit=[&](const auto& self,const auto& list)->void{for(const auto& chunk:list){if(chunk.tag==0x34201){
  if(!retail_track_info_.empty())throw std::logic_error("Multiple authored track catalogs");
  if(chunk.size%0xa4)throw std::out_of_range("Invalid Retail TrackInfo record span");
  retail_track_info_.assign(bytes.begin()+chunk.payload_offset(),bytes.begin()+chunk.payload_offset()+chunk.size);
 }self(self,chunk.children);}};visit(visit,chunks);
 // GetTrackInfo00153070 / Retail00153420: ordered one-based index and Name gate.
 if(number<=0 || std::size_t(number)>retail_track_info_.size()/0xa4 || number==54 || !retail_track_info_[(number-1)*0xa4])throw std::out_of_range("Selected original track has no valid catalog record");
 const auto source=std::size_t(number-1)*0xa4+0x84;
 const auto end=std::find(retail_track_info_.begin()+source,retail_track_info_.begin()+source+32,0);
 if(end==retail_track_info_.begin()+source+32)throw std::out_of_range("Unterminated authored track sound region");
 // Reserve an unmapped prefix: only the reviewed field is readable. Persistent
 // Memory allocations advance monotonically; releasing this pad does not rewind.
 const auto prefix=memory_.allocate(0x80);memory_.release(prefix);
 const auto region=memory_.allocate(36);allocations_.push_back(region);track_sound_info_=region-0x7c;selected_track_=number;
 std::memcpy(memory_.view(region,4).data,retail_track_info_.data()+std::size_t(number-1)*0xa4+0x7c,4);
 std::memcpy(memory_.view(region+4,32).data,retail_track_info_.data()+source,32);memory_.word(0x327398,track_sound_info_);
 return {retail_track_info_.begin()+source,end};
}
std::uint32_t CarSoundResources::track_info(int number)const{if(!track_sound_info_ || number!=selected_track_)hp2::not_implemented("0x00153070","sound-only TrackInfo owner does not contain the requested reviewed selected record");return track_sound_info_;}
void CarSoundResources::enable_track_hud_info(){
 if(track_hud_fields_)return;
 if(!track_sound_info_||selected_track_<=0)throw std::logic_error("Track HUD fields require the actual selected TrackInfo owner");
 // Debug TrackInfo datatype and Retail001BCA50 confirm calibration+5c/+60/+64;
 // constructor001B806C reads TrackNumber+41. The reviewed common header ends
 // after FinishLineAngle+6c; later version-specific fields stay unmapped.
 const auto first=std::size_t(selected_track_-1)*0xa4;
 memory_.map(track_sound_info_,std::vector<std::uint8_t>(retail_track_info_.begin()+first,retail_track_info_.begin()+first+0x6e));
 track_hud_fields_=true;
}
bool CarSoundResources::source_ready()const{return initialized_ && hp2::debug::bSound(memory_,0x32d770).Region("InGame")!=0;}
void CarSoundResources::register_sound(std::uint32_t at){sounds_.push_back(at);}
void CarSoundResources::unregister_sound(std::uint32_t at){const auto it=std::find(sounds_.begin(),sounds_.end(),at);if(it==sounds_.end())throw std::logic_error("Original car sound is not owned by these authored sources");sounds_.erase(it);}
void CarSoundResources::stop_resident_samples(){
 if(!software_voices_)hp2::not_implemented("0x0026C730","resident sample teardown requires actual SDK software voice state");
 sounds_.erase(std::remove_if(sounds_.begin(),sounds_.end(),[&](std::uint32_t at){try{memory_.view(at,1);return false;}catch(const std::out_of_range&){return true;}}),sounds_.end());
 if(!sounds_.empty())throw std::logic_error("Game car sound objects must close before resident sample teardown");
 for(auto bank=banks_.rbegin();bank!=banks_.rend();++bank)
  for(auto sample=bank->samples.rbegin();sample!=bank->samples.rend();++sample)
   hp2::debug::bSample(memory_,*sample).Stop();
}
void CarSoundResources::close(){
 // Persistent logical allocations are not reused by Memory. The no-owner
 // original destructor can release an object before its source owner closes.
 sounds_.erase(std::remove_if(sounds_.begin(),sounds_.end(),[&](std::uint32_t at){try{memory_.view(at,1);return false;}catch(const std::out_of_range&){return true;}}),sounds_.end());
 if(!sounds_.empty())throw std::logic_error("Original car sounds still borrow authored sample sources");
 if(software_voices_)for(unsigned i=0;i<48;++i)if(memory_.word(0x31ed4c+i*8))throw std::logic_error("Original software voices still borrow authored sample sources");
 if(software_voices_&&memory_.word(0x3291b8))hp2::not_implemented("0x00264CF0","queued original sound commands must be consumed before unloading their authored sources");
 for(auto bank=banks_.rbegin();bank!=banks_.rend();++bank)for(auto sample=bank->samples.rbegin();sample!=bank->samples.rend();++sample)hp2::debug::bSample(memory_,*sample).Sub_0026B3F8(3);
 if(track_sound_info_){if(memory_.word(0x327398)!=track_sound_info_)throw std::logic_error("CurrentTrackInfo owner changed while sound resources were live");if(track_hud_fields_){memory_.unmap(track_sound_info_,0x6e);track_hud_fields_=false;}memory_.word(0x327398,0);track_sound_info_=0;selected_track_=0;}retail_track_info_.clear();
 if(software_voices_){memory_.word(0x3290d4,0);memory_.word(0x3290d8,0);memory_.half(0x3290e6,0);memory_.half(0x3290e8,0);software_voices_=false;}
 initialize_vector(0x32d770+0x158);memory_.word(0x327f24,0);for(auto it=allocations_.rbegin();it!=allocations_.rend();++it)memory_.release(*it);allocations_.clear();banks_.clear();definitions_[0].clear();definitions_[1].clear();initialized_=false;decoded_patches_.clear();
}
}
