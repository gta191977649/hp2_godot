#pragma once
// Experimental Godot-only conservative occluders from the actual submitted opaque geometry.
#include "game_core/port/track/runtime.hpp"
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/occluder_instance3d.hpp>
#include <godot_cpp/classes/array_occluder3d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <map>
namespace port::godot_bridge {
inline bool eligible_track_opaque_geometry(const port::track::DrawCommand& c){return !c.effect.alpha_pass&&!c.effect.environment_pass&&!c.effect.rewrite_alpha&&!c.effect.second_pass_320&&!c.effect.matrix_pack&&!c.effect.texture.additive&&c.effect.texture.blend==port::vehicle::BlendMode::Copy&&c.effect.test.write_depth&&c.effect.test.alpha_test==1&&c.effect.test.depth_test==2;}
class TrackOcclusionPool {
 struct Slot {godot::OccluderInstance3D* node=nullptr;std::uint64_t used=0,mesh=0;};
 godot::Node3D& owner_;std::map<std::size_t,Slot> slots_;std::map<std::uint64_t,godot::Ref<godot::ArrayOccluder3D>> resources_;std::uint64_t generation_=0;
public:
 explicit TrackOcclusionPool(godot::Node3D& owner):owner_(owner){}
 ~TrackOcclusionPool(){clear();}
 void begin_frame(){++generation_;}
 void submit(std::size_t key,const godot::Ref<godot::ArrayMesh>& mesh,const godot::Transform3D& transform){
  const auto id=mesh->get_instance_id();auto& resource=resources_[id];if(resource.is_null()){const auto arrays=mesh->surface_get_arrays(0);const godot::PackedVector3Array vertices=arrays[godot::Mesh::ARRAY_VERTEX];const godot::PackedInt32Array indices=arrays[godot::Mesh::ARRAY_INDEX];resource.instantiate();resource->set_arrays(vertices,indices);}
  auto& slot=slots_[key];if(!slot.node){slot.node=memnew(godot::OccluderInstance3D);slot.node->set_layer_mask(1);owner_.add_child(slot.node);}if(slot.mesh!=id){slot.node->set_occluder(resource);slot.mesh=id;}if(slot.node->get_transform()!=transform)slot.node->set_transform(transform);if(!slot.node->is_visible())slot.node->show();slot.used=generation_;
 }
 void end_frame(){for(auto& item:slots_)if(item.second.used!=generation_&&item.second.node->is_visible())item.second.node->hide();}
 void clear(){for(auto& item:slots_){auto* n=item.second.node;if(n->get_parent())n->get_parent()->remove_child(n);memdelete(n);}slots_.clear();resources_.clear();}
 godot::Dictionary stats()const{godot::Dictionary d;d["nodes"]=int64_t(slots_.size());d["resources"]=int64_t(resources_.size());return d;}
};
}
