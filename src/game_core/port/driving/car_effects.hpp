#pragma once
// TASK-0075 E-0075-C01 + TASK0089 E0089W05: Debug0011E4E0 bindings.
#include "memory.hpp"
#include <functional>
namespace port::track {class ParticleSimulation;}
namespace port::driving {
struct CarEffectEmitter {std::size_t index=0;std::array<float,16> local_world{};unsigned car_position=0;};
struct CarEffectServices {
 bool render_info_available=false;
 std::uint32_t override_view_bits=0;
 std::function<std::vector<CarEffectEmitter>&(int,unsigned)> emitter_list;
 std::function<std::array<float,3>(unsigned)> wheel_position;
 std::function<void(const CarEffectEmitter&,float,const std::array<float,16>&,
                    const std::array<float,4>&)> emit;
};
// Port-owned terrain-list/index bindings, one instance per ParticleSimulation
// lifetime. No retained simulation pointer; original copies are shared by wheels.
class WheelParticleEffects {
public:
 void prepare(port::track::ParticleSimulation&,Memory&);
 void trigger(Memory&,std::uint32_t,float,port::track::ParticleSimulation&,
              const std::array<std::array<float,3>,3>&,
              const std::function<unsigned(Memory&,std::uint32_t)>& body_terrain={});
private:
 bool prepared_=false;
 std::array<std::array<std::vector<CarEffectEmitter>,33>,4> emitters_;
};
}
