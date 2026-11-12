// Port bundle lifetime; recovered resource/model logic remains in original owners.
#include "car_render_resources.hpp"
#include "damage_resources.hpp"
#include "game_core/port/assets/elab_chunks.hpp"
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <functional>
#include <cstdio>
#include "game_core/indep/src/fmenus.hpp"
namespace port::race {
std::string render_resource_text(Memory&m,std::uint32_t p){std::string result;while(m.byte(p)){if(result.size()==4096)throw std::runtime_error("Unterminated original render resource string");result+=char(m.byte(p++));}return result;}
std::uint32_t render_format_hash(Memory&m,std::uint32_t format,const std::string&text,int first,int second){const auto fmt=render_resource_text(m,format);char buffer[64];const auto count=std::snprintf(buffer,sizeof(buffer),fmt.c_str(),text.c_str(),first,second);if(count<0||count>=64)throw std::runtime_error("Original car render resource name exceeds64bytes");return hp2::frontend::bStringHash(buffer);}

void initialize_solid_model_lists(Memory&);
void initialize_light_material_list(Memory&);
void initialize_car_info_tables(Memory&);
void initialize_render_resource_state(Memory&m){initialize_solid_model_lists(m);initialize_light_material_list(m);initialize_car_info_tables(m);}
void CarRenderResources::load(const std::filesystem::path&path){
 std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("Cannot open original car rendering bundle");
 std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(f)),{});load_bundle(bytes);
}
void CarRenderResources::load_bundle(const std::vector<std::uint8_t>& bytes){
 std::size_t consumed=0;
 const auto tree=hp2::assets::parse_chunks_top(bytes,&consumed);if(consumed!=bytes.size())throw std::runtime_error("Incomplete original car rendering bundle");
 if(bytes.empty()||bytes.size()>0xffffffffu)throw std::runtime_error("Invalid original car rendering bundle extent");
 const auto base=m_.allocate(static_cast<std::uint32_t>(bytes.size()),16);allocations_.push_back(base);
 for(std::size_t i=0;i<bytes.size();++i)m_.byte(base+static_cast<std::uint32_t>(i),bytes[i]);
 std::function<void(const std::vector<hp2::assets::Chunk>&)> visit=[&](const auto&siblings){for(const auto&c:siblings){
  const auto chunk=base+static_cast<std::uint32_t>(c.header_offset);
  switch(c.tag){
  case 0x80034000:LoaderSolidList(m_,chunk);chunks_.push_back(chunk);break;
  case 0x3401e:LoaderLightMaterialChunk(m_,chunk);chunks_.push_back(chunk);break;
  case 0x34013:hp2::debug::LoaderPositionMarkers(m_,chunk);chunks_.push_back(chunk);break;
  case 0x34030:hp2::debug::LoaderBodyDamage(m_,chunk);chunks_.push_back(chunk);break;
  case 0x34600:case 0x34601:LoaderCarInfo(m_,chunk);chunks_.push_back(chunk);break;
  default:visit(c.children);break;
  }
 }};visit(tree);
}
// Port active-ride resource binding of recovered00167DC0 duplication requests;
// the caller owns eliminated-driver filtering and GameFlow transition.
void CarRenderResources::prepare_ride(std::uint32_t ride){
 const int instance=m_.signed_byte(ride+4);if(instance<=0)return;
 const auto info=m_.word(0x3278e0)+0x560*m_.word(ride);const auto base=render_resource_text(m_,info+32);
 if(m_.signed_byte(ride+7))for(char level='A';level<'C';++level){const auto name=base+"_"+level;eDuplicateSolid(m_,name,name+std::to_string(instance));}
 eDuplicateSolid(m_,base+"_SCUFFS",base+"_SCUFFS"+std::to_string(instance));
}
void CarRenderResources::close(){
 for(auto it=chunks_.rbegin();it!=chunks_.rend();++it){const auto chunk=*it;switch(m_.word(chunk)){
 case 0x80034000:UnloaderSolidList(m_,chunk);break;
 case 0x3401e:UnloadLightMaterialChunk(m_,chunk);break;
 case 0x34013:hp2::debug::UnloaderPositionMarkers(m_,chunk);break;
 case 0x34030:hp2::debug::UnloaderBodyDamage(m_,chunk);break;
 case 0x34600:case 0x34601:UnloaderCarInfo(m_,chunk);break;
 }}
 chunks_.clear();for(auto it=allocations_.rbegin();it!=allocations_.rend();++it)m_.release(*it);allocations_.clear();
}
}
