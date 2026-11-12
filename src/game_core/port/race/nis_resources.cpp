#include "nis_resources.hpp"
#include "game_core/port/assets/elab_chunks.hpp"
#include "game_core/port/not_implemented.hpp"
#include <ctime>
#include <stdexcept>
namespace port::race {
NisResources::NisResources(driving::Memory& m,assets::Source& source,std::function<int(driving::Memory&,std::uint32_t)> loader,std::function<int(driving::Memory&,std::uint32_t)> unloader):memory_(m),source_(source),load_camera_(std::move(loader)),unload_camera_(std::move(unloader)){
 if(!m.word(0x327da8)&&!m.word(0x327dac))hp2::debug::Sub_001A53E8(m);
}
std::uint32_t NisResources::load(const std::string& name){
 if(name.rfind("NIS/",0)!=0&&name.rfind("NIS\\",0)!=0)throw std::invalid_argument("Original NIS owner refuses non-NIS source path");
 auto canonical=name;for(auto& c:canonical)if(c=='\\')c='/';const auto bytes=source_.read(canonical);std::size_t consumed=0;const auto chunks=hp2::assets::parse_chunks_top(bytes,&consumed);if(bytes.empty()||bytes.size()>0xffffffffu||consumed!=bytes.size())throw std::runtime_error("Original NIS resource chunk stream is incomplete");
 File file;file.base=memory_.allocate(unsigned(bytes.size()),16);for(unsigned i=0;i<bytes.size();++i)memory_.byte(file.base+i,bytes[i]);
 // The token is opaque at the port LoadResourceFile/UnloadResourceFile seam;
 // no ResourceFile layout is invented or handed to unrelated source consumers.
 const auto key=file.base;files_.emplace(key,std::move(file));auto& owned=files_.at(key);
 try{const auto visit=[&](const auto& self,const auto& children)->void{for(const auto& child:children){const auto address=owned.base+unsigned(child.header_offset);if(child.tag==0x80034008){if(!hp2::debug::LoaderAnimations(memory_,address))throw std::logic_error("Original animation loader rejected NIS chunk");owned.animations.push_back(address);}else if(child.tag==0x80034420){if(!load_camera_)hp2::not_implemented("0x00197770","actual NIS camera chunk loader is unbound");if(!load_camera_(memory_,address))throw std::logic_error("Original camera loader rejected NIS CAM chunk");owned.cameras.push_back(address);}else if(child.tag==0){}else if(!child.children.empty())self(self,child.children);else hp2::not_implemented("0x001AEB18","actual NIS resource has an unreviewed nonpadding chunk");}};visit(visit,chunks);if(owned.animations.empty()&&owned.cameras.empty())throw std::runtime_error("NIS source file contained no consumed animation/camera chunk");}catch(...){unload(key);throw;}return key;
}
void NisResources::unload(std::uint32_t key){const auto found=files_.find(key);if(found==files_.end())throw std::logic_error("Stale NIS resource owner token");auto& file=found->second;for(auto i=file.cameras.rbegin();i!=file.cameras.rend();++i){if(!unload_camera_)hp2::not_implemented("0x00197D60","actual NIS CAM unloader is unbound");unload_camera_(memory_,*i);}for(auto i=file.animations.rbegin();i!=file.animations.rend();++i)hp2::debug::UnloaderAnimations(memory_,*i);memory_.release(file.base);files_.erase(found);}
void NisResources::close(){while(!files_.empty())unload(files_.begin()->first);}
std::optional<NisClock> host_nis_clock(){const auto stamp=std::time(nullptr);std::tm local{};
#ifdef _WIN32
 if(localtime_s(&local,&stamp))return {};
#else
 if(!localtime_r(&stamp,&local))return {};
#endif
 return NisClock{local.tm_mon+1,local.tm_mday,(local.tm_year+1900)%100,local.tm_hour,local.tm_min,local.tm_sec};}
}
