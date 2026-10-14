#include "catalog.hpp"
#include <fstream>
#include <filesystem>
#include <stdexcept>
namespace port::assets {
std::vector<std::uint8_t> DirectorySource::read(const std::string& path){
 if(path.empty()||path.front()=='/'||path.find("..")!=std::string::npos||path.find(':')!=std::string::npos||path.find('\\')!=std::string::npos)throw std::runtime_error("asset path must be relative to configured root");
 std::ifstream f(root_+"/"+path,std::ios::binary);if(!f)throw std::runtime_error("could not open original asset: "+path);
 return {(std::istreambuf_iterator<char>(f)),{}};
}
bool DirectorySource::probe(const std::string& path){
 if(path.empty()||path.front()=='/'||path.find("..")!=std::string::npos||path.find(':')!=std::string::npos||path.find('\\')!=std::string::npos)throw std::runtime_error("asset path must be relative to configured root");
 std::error_code error;const auto file=std::filesystem::path(root_)/path;
 return std::filesystem::is_regular_file(file,error)&&std::filesystem::file_size(file,error)>0&&!error;
}
std::shared_ptr<const render::Bitmap> Catalog::texture(render::TextureId id){
 auto cached=textures_.find(id);if(cached!=textures_.end())return cached->second;
 if(id>0xffffffffu)return {};
 for(const auto& bundle:bundles_){
  auto it=archives_.find(bundle);
  if(it==archives_.end()){
   std::vector<std::uint8_t> bytes;
   try{bytes=source_.read(bundle);}catch(const std::exception&){continue;}
   auto archive=hp2::assets::Hp2TpkArchive::load(std::move(bytes));if(!archive)continue;
   it=archives_.emplace(bundle,std::move(*archive)).first;
  }
  for(const auto& h:it->second.headers())if(h.tex_hash==id){
   auto decoded=it->second.decode(h,true);if(!decoded)continue;
   auto bitmap=std::make_shared<render::Bitmap>();bitmap->width=decoded->width;bitmap->height=decoded->height;bitmap->rgba=std::move(decoded->rgba);
   textures_[id]=bitmap;return bitmap;
  }
 }
 textures_[id]={};return {};
}
}
