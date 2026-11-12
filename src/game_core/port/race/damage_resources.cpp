// Port raw-bundle lifetime adapter; recovered loaders are in original source owners.
#include "damage_resources.hpp"
#include "game_core/port/assets/elab_chunks.hpp"
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <functional>
namespace port::race {
void initialize_body_damage_lists(port::driving::Memory&);
void initialize_position_marker_list(port::driving::Memory&);
void initialize_damage_resource_lists(port::driving::Memory&m){initialize_body_damage_lists(m);initialize_position_marker_list(m);}

void DamageResourceBindings::load(const std::filesystem::path&path){
 std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("Cannot open original damage/marker bundle");
 std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(f)),{});
 std::size_t consumed=0;const auto tree=hp2::assets::parse_chunks_top(bytes,&consumed);
 if(consumed!=bytes.size())throw std::runtime_error("Damage/marker bundle is not a complete eLab stream");
 if(bytes.size()>0xffffffffu)throw std::runtime_error("Damage/marker bundle exceeds logical address space");
 const auto base=m_.allocate(static_cast<std::uint32_t>(bytes.size()),16);allocations_.push_back(base);
 for(std::size_t i=0;i<bytes.size();++i)m_.byte(base+static_cast<std::uint32_t>(i),bytes[i]);
 std::function<void(const std::vector<hp2::assets::Chunk>&)> visit=[&](const auto&siblings){for(const auto&c:siblings){
  if(c.tag==0x34013||c.tag==0x34030){
   const auto chunk=base+static_cast<std::uint32_t>(c.header_offset);
   if(c.tag==0x34013){
    const auto pad=((chunk+0x17)&~15u)-(chunk+8);
    if(c.size<pad||(c.size-pad)%32)throw std::runtime_error("Original marker chunk has invalid record extent");
    hp2::debug::LoaderPositionMarkers(m_,chunk);
   }else{
    if(c.size<84)throw std::runtime_error("Original BodyDamageInfo header is truncated");
    hp2::debug::LoaderBodyDamage(m_,chunk);
   }
   chunks_.push_back(chunk);
  }
  visit(c.children);
 }};visit(tree);
}
void DamageResourceBindings::close(){
 for(auto it=chunks_.rbegin();it!=chunks_.rend();++it){const auto p=*it;
  if(m_.word(p)==0x34013)hp2::debug::UnloaderPositionMarkers(m_,p);else hp2::debug::UnloaderBodyDamage(m_,p);
 }
 chunks_.clear();for(auto it=allocations_.rbegin();it!=allocations_.rend();++it)m_.release(*it);allocations_.clear();
}
}
