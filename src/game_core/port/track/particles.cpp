#include "particles.hpp"
#include <cstring>
#include <cmath>
#include <stdexcept>
#include <algorithm>
namespace port::track {
namespace {
struct Read {const std::vector<std::uint8_t>& b;
void bound(std::size_t p,std::size_t n)const{if(p>b.size()||n>b.size()-p)throw std::runtime_error("Truncated original AcidFX record");}
std::uint32_t word(std::size_t p)const{bound(p,4);return b[p]|std::uint32_t(b[p+1])<<8|std::uint32_t(b[p+2])<<16|std::uint32_t(b[p+3])<<24;}
float number(std::size_t p)const{auto bits=word(p);float value;std::memcpy(&value,&bits,4);if(!std::isfinite(value))throw std::runtime_error("Nonfinite AcidFX asset value");return value;}
std::string text(std::size_t p,std::size_t n)const{bound(p,n);return {b.begin()+p,std::find(b.begin()+p,b.begin()+p+n,0)};}
template<std::size_t N>void floats(std::array<float,N>& out,std::size_t p)const{for(unsigned i=0;i<N;++i)out[i]=number(p+i*4);}
};
}
ParticleResources decode_particles(const std::vector<std::uint8_t>& bytes){Read read{bytes};ParticleResources out;
 const auto walk=[&](const auto& self,std::size_t p,std::size_t end)->void{while(p<end){const auto id=read.word(p),size=read.word(p+4);if(end-p<8||size>end-p-8)throw std::runtime_error("AcidFX chunk exceeds container");const auto finish=p+8+size;
 if(id==0x35021||id==0x35020){const auto a=(p+23)&~std::size_t(15);if(a>finish||finish-a<16)throw std::runtime_error("Truncated AcidFX header");const unsigned version=id==0x35021?4:5,stride=id==0x35021?544:192;if(read.word(a+8)!=version)throw std::runtime_error("Unsupported original AcidFX version");const auto count=read.word(a+12);if(count>(finish-a-16)/stride)throw std::runtime_error("AcidFX record count exceeds chunk");
 for(unsigned i=0;i<count;++i){const auto r=a+16+i*stride;if(id==0x35021){ParticleDefinition d;d.name_hash=read.word(r+8);d.texture_hash=read.word(r+12);d.group_hash=read.word(r+16);for(unsigned k=0;k<8;++k)d.flags[k]=bytes[r+24+k];read.floats(d.controls,r+32);read.floats(d.volume_center,r+96);read.floats(d.volume_extent,r+112);d.far_clip=read.number(r+128);d.gravity=read.number(r+132);d.drag=read.number(r+136);d.max_pixel_size=read.number(r+140);read.floats(d.key_positions,r+144);read.floats(d.colour_matrix,r+160);read.floats(d.size,r+224);read.floats(d.relative_angle,r+240);read.floats(d.colour_basis,r+256);read.floats(d.extra_basis,r+320);d.name=read.text(r+384,64);d.texture_name=read.text(r+448,48);d.group_name=read.text(r+496,48);out.definitions.push_back(std::move(d));}
 else {ParticleEmitter e;e.name_hash=read.word(r+8);e.data_hash=read.word(r+12);e.state=read.word(r+16);e.cycle_time=read.number(r+20);e.countdown=read.number(r+24);e.random_seed=read.word(r+36);e.dont_emit=read.word(r+40);read.floats(e.local_world,r+48);read.floats(e.inherit_velocity,r+112);e.name=read.text(r+128,64);out.emitters.push_back(std::move(e));}}
 }else if(id&0x80000000)self(self,p+8,finish);p=finish;}};walk(walk,0,bytes.size());return out;
}
}

#include "game_core/indep/inc/AcidFX.hpp"
#include "game_core/port/not_implemented.hpp"
#include <algorithm>
namespace port::track {
ParticleSimulation::ParticleSimulation(ParticleResources resources,std::uint32_t seed):resources_(std::move(resources)),seed_(seed){
 static_count_=resources_.emitters.size();
 definition_.resize(resources_.emitters.size(),resources_.definitions.size());
 for(std::size_t i=0;i<resources_.emitters.size();++i){for(std::size_t d=0;d<resources_.definitions.size();++d)if(resources_.definitions[d].name_hash==resources_.emitters[i].data_hash){if(definition_[i]!=resources_.definitions.size())throw std::runtime_error("Ambiguous original AcidFX emitter data");definition_[i]=d;}
  const auto* data=definition_[i]<resources_.definitions.size()?&resources_.definitions[definition_[i]]:nullptr;
  hp2::recovered::AcidEffectSystem::AddAutoEmitter(resources_.emitters[i],data,seed_,[&](bool head){if(head)order_.insert(order_.begin(),i);else order_.push_back(i);});
 }
}
std::size_t ParticleSimulation::particle_count()const{std::size_t count=0;for(const auto& g:groups_)count+=g.particles.size();return count;}
void ParticleSimulation::set_gpu_emission(bool enabled){
 if(enabled==gpu_emission_)return;
 if(!groups_.empty()||!gpu_commands_.empty())throw std::logic_error("Particle backend must be selected before spawning");
 gpu_emission_=enabled;
}
std::vector<ParticleGpuCommand> ParticleSimulation::take_gpu_commands(){
 auto result=std::move(gpu_commands_);gpu_commands_.clear();return result;
}
void ParticleSimulation::capture_spawn(const ParticleGroup& group,std::size_t before){
 if(before==group.particles.size())return;
 ParticleGpuCommand command;command.kind=ParticleGpuCommandKind::Spawn;
 command.group=group.id;command.emitter=group.emitter;command.definition=group.definition;
 command.particles.assign(group.particles.begin()+before,group.particles.end());
 gpu_commands_.push_back(std::move(command));
}
void ParticleSimulation::clear_dynamic_emitters(){
 if(gpu_emission_){
  auto commands=gpu_commands_;
  for(const auto& group:groups_)if(group.emitter>=static_count_){ParticleGpuCommand command;
   command.kind=ParticleGpuCommandKind::RemoveGroup;command.group=group.id;
   command.emitter=group.emitter;command.definition=group.definition;commands.push_back(std::move(command));}
  gpu_commands_.swap(commands);
 }
 // Port-side detach: static world effects and shared RNG survive car resets.
 groups_.remove_if([&](const auto& group){return group.emitter>=static_count_;});
 resources_.emitters.resize(static_count_);definition_.resize(static_count_);
}
std::size_t ParticleSimulation::add_dynamic_emitter(const ParticleEmitter& source){
 // Debug afxCopyEmitter 0x00127380: zero allocated record, copy identity,
 // data binding and LocalWorld only. See reverse/task0075/dynamic-particles.md.
 ParticleEmitter copy;copy.name=source.name.substr(0,63);copy.name_hash=source.name_hash;
 copy.data_hash=source.data_hash;copy.local_world=source.local_world;
 std::size_t definition=resources_.definitions.size();
 for(std::size_t d=0;d<resources_.definitions.size();++d)if(resources_.definitions[d].name_hash==copy.data_hash){
  if(definition!=resources_.definitions.size())throw std::runtime_error("Ambiguous original dynamic AcidFX emitter data");
  definition=d;
 }
 if(definition==resources_.definitions.size())hp2::not_implemented("0x00127518","Dynamic AcidEmitter requires its original emitter data");
 const auto index=resources_.emitters.size();
 definition_.push_back(definition);
 try{resources_.emitters.push_back(std::move(copy));}catch(...){definition_.pop_back();throw;}
 return index;
}
void ParticleSimulation::trigger_dynamic(port::driving::Memory& math,std::size_t index,float intensity,
 const std::array<std::array<float,3>,3>& points,const std::array<float,16>& transform,const std::array<float,4>& velocity,const std::array<float,16>* restored_orientation){
 if(gpu_emission_){auto next=*this;next.trigger_gpu(math,index,intensity,points,transform,velocity,restored_orientation);*this=std::move(next);return;}
 if(std::find(order_.begin(),order_.end(),index)!=order_.end())throw std::invalid_argument("Dynamic trigger requires a copied emitter");
 auto& emitter=resources_.emitters.at(index);const auto saved=restored_orientation?*restored_orientation:emitter.local_world;
 emitter.local_world=transform;emitter.inherit_velocity=velocity;
 auto group=std::find_if(groups_.begin(),groups_.end(),[&](const auto& g){return g.emitter==index;});
 const bool available=group!=groups_.end()||groups_.size()<400;
 if(group==groups_.end()&&available){groups_.push_back({index,definition_.at(index),{}});group=std::prev(groups_.end());}
 std::vector<Particle> unavailable;auto& particles=available?group->particles:unavailable;
 const auto count=particle_count();
 try{hp2::recovered::AcidEffectSystem::TriggerEmitter(math,emitter,resources_.definitions.at(definition_.at(index)),intensity,points,seed_,particles,count<1000?1000-count:0,available);}
 catch(...){emitter.local_world=saved;throw;}
 emitter.local_world=saved;
 if(available&&particles.empty())groups_.erase(group);
}
void ParticleSimulation::step(port::driving::Memory& math,float dt,const std::array<std::array<float,3>,3>& points){
 if(gpu_emission_){auto next=*this;next.advance_gpu(math,dt,points);*this=std::move(next);return;}
 // Host transaction: failures never publish partially advanced RNG, groups or emitters.
 // Both paths invoke the same recovered calls in the same order. The candidate
 // stages only mutable state; immutable definitions and traversal indices stay put.
 const auto advance=[&](auto& emitters,auto& groups,auto& seed){
 std::size_t count=0;for(const auto& group:groups)count+=group.particles.size();
 for(auto i:order_){auto& e=emitters[i];if(e.state>=2)break;
  const auto& data=resources_.definitions.at(definition_[i]);
  hp2::recovered::AcidEffectSystem::DoTimeStep(e,data,dt,[&](float step){
   auto group=std::find_if(groups.begin(),groups.end(),[&](const auto& g){return g.emitter==i;});
   const bool available=group!=groups.end()||groups.size()<400;
   if(group==groups.end()&&available){groups.push_back({i,definition_[i],{}});group=std::prev(groups.end());}
   std::vector<Particle> unavailable;auto& particles=available?group->particles:unavailable;const auto before=particles.size();
   hp2::recovered::AcidEffectSystem::TriggerEmitter(math,e,data,step,points,seed,particles,count<1000?1000-count:0,available);count+=particles.size()-before;
   if(available&&particles.empty())groups.erase(group);
  });
 }
 // Debug00128BA4..00128BF0: static pCar==0 contributes no live motion;
 // dynamic groups use current car velocity only for positive MotionInherit.
 for(auto i=groups.begin();i!=groups.end();){const auto& data=resources_.definitions[i->definition];hp2::recovered::AcidEffectSystem::DoTimeStep(i->particles,data,dt,i->emitter>=static_count_&&data.controls[12]>0?car_velocity_:std::array<float,3>{});if(i->particles.empty())i=groups.erase(i);else ++i;}
 };
 if(!state_cache_){auto next=*this;advance(next.resources_.emitters,next.groups_,next.seed_);*this=std::move(next);return;}
 auto emitters=resources_.emitters;auto groups=groups_;auto seed=seed_;
 advance(emitters,groups,seed);
 // std::allocator-backed container swaps cannot fail, so publishing is atomic
 // with respect to exceptions just as in the whole-simulation reference path.
 resources_.emitters.swap(emitters);groups_.swap(groups);seed_=seed;
}
}

namespace port::track {
// E-0085-N01. Reuse original TriggerEmitter00127858 without changing random
// calls, group availability or shared capacity. Copy/commit belongs to caller.
void ParticleSimulation::trigger_gpu(port::driving::Memory& math,std::size_t index,float intensity,
 const std::array<std::array<float,3>,3>& points,const std::array<float,16>& transform,
 const std::array<float,4>& velocity,const std::array<float,16>* restored_orientation){
 if(std::find(order_.begin(),order_.end(),index)!=order_.end())throw std::invalid_argument("Dynamic trigger requires a copied emitter");
 auto& emitter=resources_.emitters.at(index);const auto saved=restored_orientation?*restored_orientation:emitter.local_world;
 emitter.local_world=transform;emitter.inherit_velocity=velocity;
 auto group=std::find_if(groups_.begin(),groups_.end(),[&](const auto& g){return g.emitter==index;});
 const bool available=group!=groups_.end()||groups_.size()<400;
 if(group==groups_.end()&&available){groups_.push_back({index,definition_.at(index),{},next_group_id_++});group=std::prev(groups_.end());}
 std::vector<Particle> unavailable;auto& particles=available?group->particles:unavailable;
 const auto count=particle_count(),before=particles.size();
 hp2::recovered::AcidEffectSystem::TriggerEmitter(math,emitter,resources_.definitions.at(definition_.at(index)),intensity,points,seed_,particles,count<1000?1000-count:0,available);
 emitter.local_world=saved;
 if(available){capture_spawn(*group,before);if(particles.empty())groups_.erase(group);}
}
// E-0085-N01. Emitters and life ledger retain the existing native scheduler's
// ordering. Motion, packed direction/speed, animation and appearance move to GPU.
void ParticleSimulation::advance_gpu(port::driving::Memory& math,float dt,const std::array<std::array<float,3>,3>& points){
 if(!std::isfinite(dt)||dt<0)throw std::invalid_argument("Invalid original particle timestep");
 std::size_t count=particle_count();
 for(auto i:order_){auto& emitter=resources_.emitters[i];if(emitter.state>=2)break;
  const auto& data=resources_.definitions.at(definition_[i]);
  hp2::recovered::AcidEffectSystem::DoTimeStep(emitter,data,dt,[&](float duration){
   auto group=std::find_if(groups_.begin(),groups_.end(),[&](const auto& g){return g.emitter==i;});
   const bool available=group!=groups_.end()||groups_.size()<400;
   if(group==groups_.end()&&available){groups_.push_back({i,definition_[i],{},next_group_id_++});group=std::prev(groups_.end());}
   std::vector<Particle> unavailable;auto& particles=available?group->particles:unavailable;
   const auto before=particles.size();
   hp2::recovered::AcidEffectSystem::TriggerEmitter(math,emitter,data,duration,points,seed_,particles,count<1000?1000-count:0,available);
   count+=particles.size()-before;
   if(available){capture_spawn(*group,before);if(particles.empty())groups_.erase(group);}
  });
 }
 const auto ticks=port::driving::cvt_w_s(dt*1024.f);
 for(auto group=groups_.begin();group!=groups_.end();){
  ParticleGpuCommand command;command.kind=ParticleGpuCommandKind::Advance;
  command.group=group->id;command.emitter=group->emitter;command.definition=group->definition;command.timestep=dt;
  const auto& data=resources_.definitions.at(group->definition);
  if(group->emitter>=static_count_&&data.controls[12]>0)command.inherited=car_velocity_;
  gpu_commands_.push_back(command);
  auto& particles=group->particles;
  particles.erase(std::remove_if(particles.begin(),particles.end(),[&](Particle& p){const int life=int(p.life)-ticks;if(life<1)return true;p.life=static_cast<std::int16_t>(life);return false;}),particles.end());
  if(particles.empty()){command.kind=ParticleGpuCommandKind::RemoveGroup;command.timestep=0;gpu_commands_.push_back(std::move(command));group=groups_.erase(group);}else ++group;
 }
}
}
