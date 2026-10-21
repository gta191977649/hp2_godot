#include "track_instance_pool.hpp"
#include <godot_cpp/classes/multi_mesh.hpp>
#include <map>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <stdexcept>
#include <limits>
namespace port::godot_bridge {
struct TrackInstancePool::Impl {
 struct Slot {godot::MultiMeshInstance3D* node=nullptr;godot::Ref<godot::MultiMesh> multimesh;std::vector<godot::Transform3D> transforms;std::size_t capacity=0,generation=0;};
 godot::Node3D& owner;std::map<std::pair<unsigned,std::size_t>,Slot> slots;std::size_t allocations=0,uploads=0;std::map<unsigned,std::size_t> generations;
 explicit Impl(godot::Node3D& value):owner(value){}
};
TrackInstancePool::TrackInstancePool(godot::Node3D& owner):impl_(std::make_unique<Impl>(owner)){}
TrackInstancePool::~TrackInstancePool(){clear();}
void TrackInstancePool::begin_view(unsigned view){++impl_->generations[view];}
void TrackInstancePool::end_view(unsigned view){for(auto& pair:impl_->slots)if(pair.first.first==view&&pair.second.generation!=impl_->generations[view]&&pair.second.node->is_visible())pair.second.node->hide();}
godot::MultiMeshInstance3D* TrackInstancePool::submit(unsigned view,std::size_t key,const godot::Ref<godot::Mesh>& mesh,const godot::Ref<godot::ShaderMaterial>& material,const std::vector<godot::Transform3D>& transforms,unsigned layer){
 if(mesh.is_null()||material.is_null()||transforms.size()<2||transforms.size()>std::size_t(std::numeric_limits<int>::max())||layer==0)throw std::invalid_argument("Invalid conservative track MultiMesh submission");
 for(const auto& t:transforms)if(!t.is_finite()||!t.basis.is_finite()||!(t.basis.determinant()>0))throw std::invalid_argument("Invalid track MultiMesh transform");
 auto& slot=impl_->slots[{view,key}];
 if(!slot.node){slot.node=memnew(godot::MultiMeshInstance3D);slot.node->set_cast_shadows_setting(godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);slot.multimesh.instantiate();slot.multimesh->set_transform_format(godot::MultiMesh::TRANSFORM_3D);slot.node->set_multimesh(slot.multimesh);impl_->owner.add_child(slot.node);++impl_->allocations;}
 if(slot.multimesh->get_mesh()!=mesh)slot.multimesh->set_mesh(mesh);
 if(slot.capacity<transforms.size()){std::size_t capacity=1;while(capacity<transforms.size()){if(capacity>std::size_t(std::numeric_limits<int>::max())/2)throw std::length_error("Track instance power-of-two capacity overflow");capacity*=2;}slot.multimesh->set_instance_count(int(capacity));slot.capacity=capacity;slot.transforms.clear();}
 if(slot.transforms!=transforms){godot::PackedFloat32Array buffer;buffer.resize(int64_t(slot.capacity)*12);auto* values=buffer.ptrw();for(std::size_t i=0;i<transforms.size();++i){const auto& t=transforms[i];for(unsigned row=0;row<3;++row){for(unsigned col=0;col<3;++col)values[i*12+row*4+col]=t.basis[row][col];values[i*12+row*4+3]=t.origin[row];}}slot.multimesh->set_buffer(buffer);slot.transforms=transforms;++impl_->uploads;}
 if(slot.multimesh->get_visible_instance_count()!=int(transforms.size()))slot.multimesh->set_visible_instance_count(int(transforms.size()));slot.generation=impl_->generations[view];
 if(slot.node->get_material_override()!=material)slot.node->set_material_override(material);
 if(slot.node->get_layer_mask()!=layer)slot.node->set_layer_mask(layer);if(!slot.node->is_visible())slot.node->show();return slot.node;
}
void TrackInstancePool::clear(){for(auto& pair:impl_->slots){auto* node=pair.second.node;if(node->get_parent())node->get_parent()->remove_child(node);memdelete(node);}impl_->slots.clear();impl_->generations.clear();}
std::size_t TrackInstancePool::entries()const{return impl_->slots.size();}
std::size_t TrackInstancePool::allocations()const{return impl_->allocations;}
std::size_t TrackInstancePool::transform_uploads()const{return impl_->uploads;}
std::size_t TrackInstancePool::capacity()const{std::size_t total=0;for(const auto& pair:impl_->slots)total+=pair.second.capacity;return total;}
}
