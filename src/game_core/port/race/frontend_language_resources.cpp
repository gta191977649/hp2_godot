#include "frontend_language_resources.hpp"
#include "game_core/indep/src/language.hpp"
#include "game_core/port/assets/fe_language.hpp"
#include "game_core/port/assets/elab_chunks.hpp"
#include <cstring>
namespace port::race {
void FrontendLanguageResources::load(const std::vector<std::uint8_t>& bytes){
 auto& m=memory_;if(chunk_||m.word(0x31389c))throw std::logic_error("Original language description is already owned");
 // Previously reviewed Retail0x64-byte header/8-byte signed-key tables. The
 // raw loader fixes the same consumed fields; every other source byte stays.
 hp2::port::assets::FeLanguage validate(bytes);(void)validate;
 const auto word=[&](unsigned o){return std::uint32_t(bytes[o])|(std::uint32_t(bytes[o+1])<<8)|(std::uint32_t(bytes[o+2])<<16)|(std::uint32_t(bytes[o+3])<<24);};
 const auto language=word(0x50);if(language>=8)throw std::out_of_range("Authored language enum outside original domain");
 hp2::debug::LanguageManager manager(m,0x313898);manager.Sub_00218BB8();chunk_=m.allocate(std::uint32_t(bytes.size()));std::memcpy(m.view(chunk_,bytes.size()).data,bytes.data(),bytes.size());hp2::debug::LoaderLanguage(m,chunk_);manager.SetCurrLanguage(int(language));
}
void FrontendLanguageResources::load_track_names(const std::vector<std::uint8_t>& bytes){auto& m=memory_;if(track_table_||m.word(0x32739c))throw std::logic_error("Original TrackInfoTable already has an owner");const auto chunks=hp2::assets::parse_chunks_top(bytes);const auto* table=hp2::assets::find_first(chunks,0x34201);if(!table||table->size%0xa4)throw std::runtime_error("Missing/misaligned authored TrackInfo catalog");retail_tracks_.assign(bytes.begin()+table->payload_offset(),bytes.begin()+table->payload_offset()+table->size);const auto count=std::uint32_t(retail_tracks_.size()/0xa4);
 for(unsigned i=0;i<count;++i){const auto name=m.allocate(32);if(!i)track_table_=name;if(name!=track_table_+i*0xa0)throw std::logic_error("Selective TrackInfo row stride was not preserved");track_allocations_.push_back(name);const auto gap=m.allocate(8,1);m.release(gap);const auto ids=m.allocate(8,1);track_allocations_.push_back(ids);if(ids!=name+0x48)throw std::logic_error("Selective TrackInfo ID offset was not preserved");const auto tail=m.allocate(0x30,1);m.release(tail);
  std::memcpy(m.view(name,32).data,retail_tracks_.data()+i*0xa4,32);std::memcpy(m.view(ids,8).data,retail_tracks_.data()+i*0xa4+0x48,8);
  // Independent Retail185640/Debug183A08 both consume CopCharacterNameHash54.
  // A separately tracked4byte projection leaves unknown Length50 unreadable.
  m.map(name+0x54,std::vector<std::uint8_t>(retail_tracks_.begin()+i*0xa4+0x54,retail_tracks_.begin()+i*0xa4+0x58));track_cop_fields_.push_back(name+0x54);
  m.map(name+0x43,std::vector<std::uint8_t>{retail_tracks_[i*0xa4+0x43]});track_point_fields_.push_back(name+0x43);
  // E-0099-W84: Retail00186708 and Debug00184A78 independently consume
  // IsValid40, TrackNumber41, SameAsTrackNumber42 and HighScoreIndex46.
  m.map(name+0x40,std::vector<std::uint8_t>(retail_tracks_.begin()+i*0xa4+0x40,retail_tracks_.begin()+i*0xa4+0x43));track_valid_fields_.push_back(name+0x40);
  m.map(name+0x46,std::vector<std::uint8_t>(retail_tracks_.begin()+i*0xa4+0x46,retail_tracks_.begin()+i*0xa4+0x48));track_score_fields_.push_back(name+0x46);
  // Debug00153258 calls GetTrackInfo(54), then clears the entire A0 record
  // only when its original Name was present. Clear every reviewed projection;
  // unreviewed holes stay unmapped rather than publishing invented zeros.
  if(i==53&&m.byte(name)){for(unsigned j=0;j<32;++j)m.byte(name+j,0);m.word(ids,0);m.word(ids+4,0);m.byte(name+0x43,0);m.word(name+0x54,0);for(unsigned j=0;j<3;++j)m.byte(name+0x40+j,0);m.half(name+0x46,0);}
 }m.word(0x32739c,track_table_);m.word(0x3273a0,count);
}
void FrontendLanguageResources::close(){if(track_table_){if(memory_.word(0x32739c)!=track_table_)throw std::logic_error("Original TrackInfoTable owner changed");memory_.word(0x32739c,0);memory_.word(0x3273a0,0);for(auto field:track_cop_fields_)memory_.unmap(field,4);track_cop_fields_.clear();for(auto field:track_point_fields_)memory_.unmap(field,1);track_point_fields_.clear();for(auto field:track_valid_fields_)memory_.unmap(field,3);track_valid_fields_.clear();for(auto field:track_score_fields_)memory_.unmap(field,2);track_score_fields_.clear();for(auto allocation:track_allocations_)memory_.release(allocation);track_allocations_.clear();track_table_=0;retail_tracks_.clear();}if(!chunk_)return;hp2::debug::LanguageManager(memory_,0x313898).Unloader(chunk_);memory_.release(chunk_);chunk_=0;}
}
