#include "runtime.hpp"
#include "game_core/src/ecstasy/ecstasye.hpp"
#include <cmath>
#include <numeric>
#include <stdexcept>
namespace port::track {
void Runtime::load(std::shared_ptr<const Scene> scene){if(!scene)throw std::invalid_argument("Null track runtime scene");std::vector<std::size_t> order(scene->zones.size());std::iota(order.begin(),order.end(),0);scene_=std::move(scene);zone_order_=std::move(order);texture_chain_order_.clear();}
void Runtime::load(assets::Source& source,const Entry& entry){auto scene=std::make_shared<const Scene>(load_scene(source,entry));load(std::move(scene));}
void Runtime::unload(){scene_.reset();zone_order_.clear();texture_chain_order_.clear();}
Frame Runtime::prepare_frame(const FrameView& view,port::driving::Memory& math){
    if(!scene_)throw std::logic_error("Track runtime has no loaded resources");
    for(auto v:view.position)if(!std::isfinite(v))throw std::invalid_argument("Nonfinite track camera position");for(auto v:view.direction)if(!std::isfinite(v))throw std::invalid_argument("Nonfinite track camera direction");for(auto v:view.clip_matrix)if(!std::isfinite(v))throw std::invalid_argument("Nonfinite track clip matrix");if(!std::isfinite(view.focal_h)||view.focal_h<=0)throw std::invalid_argument("Invalid track horizontal focal length");
    auto order=zone_order_;auto texture_order=texture_chain_order_;auto result=hp2::recovered::eDisplayFrame(*scene_,view,order,math,&texture_order);zone_order_=std::move(order);texture_chain_order_=std::move(texture_order);return result;
}
std::string Runtime::diagnostics()const{return scene_?"Track "+std::to_string(scene_->entry.number)+": "+std::to_string(scene_->sections.size())+" scenery sections, "+std::to_string(scene_->zones.size())+" visible zones":"Track runtime unloaded";}
}
