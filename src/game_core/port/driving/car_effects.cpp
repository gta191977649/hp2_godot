#include "car_effects.hpp"
#include "terrain_effects.hpp"
#include "game_core/indep/inc/Car.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include "game_core/port/track/particles.hpp"
#include "game_core/port/not_implemented.hpp"
#include <stdexcept>
#include <cstring>
namespace port::driving {
namespace {
std::string original_text(const Memory& m,std::uint32_t p){std::string s;if(!p)return s;for(unsigned i=0;i<4096;++i){const auto c=m.byte(p+i);if(!c)return s;s.push_back(static_cast<char>(c));}throw std::runtime_error("Unterminated original terrain emitter group");}
}
// Debug0021CB40,00127438,001272F0; E-0075-C01. Binding/storage only.
void WheelParticleEffects::prepare(port::track::ParticleSimulation& simulation,Memory& m){
 if(prepared_)return;
 const auto sources=simulation.resources().emitters;
 const auto definitions=simulation.resources().definitions;
 const auto order=simulation.auto_emitter_order();
 for(unsigned terrain=0;terrain<33;++terrain)for(unsigned effect=0;effect<4;++effect){
  constexpr unsigned fields[]{0x3c,0x48,0x54,0x60};
  const auto name=original_text(m,m.word(0x3102e8+terrain*0x90+fields[effect]));
  if(name.empty())continue;
  const auto hash=hp2::frontend::bStringHash(name.c_str());
  for(const auto source_index:order){const auto& source=sources.at(source_index);
   const port::track::ParticleDefinition* definition=nullptr;
   for(const auto& d:definitions)if(d.name_hash==source.data_hash){if(definition)throw std::runtime_error("Ambiguous original terrain emitter definition");definition=&d;}
   if(!definition||definition->group_hash!=hash)continue;
   const auto index=simulation.add_dynamic_emitter(source);
   unsigned position;static_assert(sizeof(position)==4);std::memcpy(&position,&definition->controls[14],4);
   emitters_[effect][terrain].push_back({index,source.local_world,position});
  }
 }
 prepared_=true;
}
void WheelParticleEffects::trigger(Memory& m,std::uint32_t car,float distance,port::track::ParticleSimulation& simulation,const std::array<std::array<float,3>,3>& interests,const std::function<unsigned(Memory&,std::uint32_t)>& body_terrain){
 prepare(simulation,m);
 CarEffectServices effects;
 // Host render resources are loaded before this adapter is called. Logical Car
 // has no target CarRenderInfo allocation; expose that availability explicitly.
 effects.render_info_available=true;
 effects.override_view_bits=m.word(car+0x510);
 effects.emitter_list=[&](int effect,unsigned terrain)->std::vector<CarEffectEmitter>&{if(terrain>=33)hp2::not_implemented("0x0011E4E0","Terrain type outside recovered emitter table");return emitters_.at(static_cast<unsigned>(effect-1)).at(terrain);};
 effects.wheel_position=[&](unsigned wheel){const auto cfg=m.word(car+0xc)+0x120+wheel*0x20;return std::array<float,3>{{m.scalar(cfg),m.scalar(cfg+4),m.scalar(cfg+8)}};};
 effects.emit=[&](const CarEffectEmitter& emitter,float step,const auto& matrix,const auto& velocity){simulation.trigger_dynamic(m,emitter.index,step,interests,matrix,velocity,&emitter.local_world);};
 TerrainEffectServices terrain;
 terrain.trigger=[&](auto address,int effect,float intensity,int parameter,const auto* velocity){hp2::recovered::Car(m,address).TriggerEffect(effect,intensity,parameter,velocity,effects);};
 trigger_wheel_terrain_effects(m,car,distance,terrain);
 unsigned grounded=0;for(unsigned i=0;i<4;++i)grounded+=m.byte(car+0x350+i)!=0;
 if(grounded>2){
  if(!body_terrain)hp2::not_implemented("0x00133988","DriveOver requires original body terrain query binding");
  trigger_drive_over_effects(m,car,body_terrain(m,car),terrain);
 }
}
}
