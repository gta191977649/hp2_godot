#pragma once
#include "game_core/assets/hp2_tpk_texture.hpp"
#include "port/render/frame.hpp"
#include <map>
#include <string>
namespace port::assets {
// File access is injected; render resources have no Godot objects or GPU handles.
class Source { public: virtual ~Source()=default; virtual std::vector<std::uint8_t> read(const std::string& path)=0; virtual const std::string& root()const=0; };
class DirectorySource final:public Source {
 std::string root_;
public:
 explicit DirectorySource(std::string root):root_(std::move(root)){}
 std::vector<std::uint8_t> read(const std::string& path)override;
 const std::string& root()const override{return root_;}
};
class Catalog:public render::Resources {
 Source& source_;
 std::map<std::string,hp2::assets::Hp2TpkArchive> archives_;
 std::map<render::TextureId,std::shared_ptr<render::Bitmap>> textures_;
 std::vector<std::string> bundles_{"FRONTEND/FRONTB.BUN","GLOBAL/GLOBALB.BUN","GLOBAL/INGAMEB.BUN","GLOBAL/HUDTEX.BIN","GLOBAL/DYNTEX.BIN","FRONTEND/TSELECT.BIN"};
public:
 explicit Catalog(Source& source):source_(source){}
 std::shared_ptr<const render::Bitmap> texture(render::TextureId id)override;
 void update(render::TextureId id,std::shared_ptr<render::Bitmap> bitmap){textures_[id]=std::move(bitmap);}
 void remove(render::TextureId id){textures_.erase(id);}
 void reset(){textures_.clear();archives_.clear();}
};
}
