#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <list>
#include <functional>
#include "game_core/port/driving/memory.hpp"
namespace port::track {
// Port execution budget, not an original emission limit; exhaustion throws.
inline constexpr std::size_t emission_iteration_budget=1u<<20;
// Explicit host projections of original AcidFX records, never target pointers.
struct ParticleDefinition {
 std::uint32_t name_hash=0,texture_hash=0,group_hash=0;std::string name,texture_name,group_name;
 std::array<std::uint8_t,8> flags{};
 std::array<float,16> controls{}; // original floats+32..92, including pad2
 std::array<float,4> volume_center{},volume_extent{},key_positions{},size{},relative_angle{};
 float far_clip=0,gravity=0,drag=0,max_pixel_size=0;
 std::array<float,16> colour_matrix{},colour_basis{},extra_basis{};
};
struct ParticleEmitter {
 std::uint32_t name_hash=0,data_hash=0,state=0,random_seed=0,dont_emit=0;
 float cycle_time=0,countdown=0;std::array<float,16> local_world{};
 std::array<float,4> inherit_velocity{};std::string name;
};
struct ParticleResources {std::vector<ParticleDefinition> definitions;std::vector<ParticleEmitter> emitters;};
struct Particle {
 std::int16_t life=0;std::uint16_t angle=0,speed=0;std::uint8_t inherit=0,frame=0;
 std::array<float,3> position{};std::array<std::int8_t,3> direction{};
};
struct ParticleProjection {float camera_z=0,screen_x=0,screen_y=0;std::uint32_t screen_depth_bits=0;};
struct ParticleView {
 float focal_h=0;unsigned z_shift=4;
 // Exact original eGetClipFlags / world-view / eRotTransPers boundaries.
 std::function<bool(const std::array<float,3>&)> outside_clip;
 std::function<ParticleProjection(const std::array<float,3>&)> project;
 std::function<bool(std::uint32_t)> texture_available;
};
struct ParticleQuad {
 std::array<std::array<std::int32_t,2>,4> xy{}; // original GS12.4 screen coordinates
 std::array<std::array<float,2>,4> uv{};std::array<std::uint8_t,4> colour{};
 std::uint32_t native_depth=0;
};
struct ParticleDraw {std::uint32_t texture=0;std::size_t group=0;float depth=0;std::vector<ParticleQuad> quads;};
struct ParticleGroup {std::size_t emitter=0,definition=0;std::vector<Particle> particles;std::uint64_t id=0;};
// TASK0085 E-0085-N01: port transfer commands, not original game objects.
// Static emission precedes Advance; externally triggered car emission follows it.
enum class ParticleGpuCommandKind {Spawn,Advance,RemoveGroup};
struct ParticleGpuCommand {
 ParticleGpuCommandKind kind=ParticleGpuCommandKind::Spawn;
 std::uint64_t group=0;
 std::size_t emitter=0,definition=0;
 float timestep=0;
 std::array<float,3> inherited{};
 std::vector<Particle> particles;
};
// Port-owned list/storage adapter for original WorldEffects and copied car emitters.
// Shared original RNG persists across load/unload at the caller's boundary.
class ParticleSimulation {
public:
 explicit ParticleSimulation(ParticleResources,std::uint32_t global_seed);
 void step(port::driving::Memory&,float,const std::array<std::array<float,3>,3>& interest_points);
 // Select before spawning. GPU mode keeps only the original life/expiry ledger
 // on CPU; group particle positions remain their birth positions in this mode.
 void set_gpu_emission(bool enabled);
 bool gpu_emission()const{return gpu_emission_;}
 const std::vector<ParticleGpuCommand>& gpu_commands()const{return gpu_commands_;}
 std::vector<ParticleGpuCommand> take_gpu_commands();
 // Port-owned copies of Debug afxCopyEmitter 0x00127380 (E-0075-001).
 // Dynamic copies share particle/group pools, but never enter the auto-emitter list.
 std::size_t add_dynamic_emitter(const ParticleEmitter&);
 void clear_dynamic_emitters();
 // Caller stages a simulation transaction around event triggers plus step().
 // Debug Car::TriggerEffect restores orientation with its last selected local
 // position (E-0075-P02), retaining velocity. Caller supplies that restoration.
 void trigger_dynamic(port::driving::Memory&,std::size_t,float,
     const std::array<std::array<float,3>,3>& interest_points,
     const std::array<float,16>& transform,const std::array<float,4>& inherited_velocity,
     const std::array<float,16>* restored_orientation=nullptr);
 const std::vector<std::size_t>& auto_emitter_order()const{return order_;}
 const std::list<ParticleGroup>& groups()const{return groups_;}
 const ParticleResources& resources()const{return resources_;}
 std::uint32_t global_seed()const{return seed_;}
 std::size_t particle_count()const;
 // E-0075-D02: active groups with pCar read the current Car.State.Velocity,
 // independently of the last emission. This adapter owns one driving car.
 void set_car_velocity(const std::array<float,3>& value){car_velocity_=value;}
 // Port-only step transaction optimization; definitions and traversal indices stay put during step.
 void set_state_cache(bool enabled){state_cache_=enabled;}
private:
 bool state_cache_=false,gpu_emission_=false;
 std::uint64_t next_group_id_=1;
 std::vector<ParticleGpuCommand> gpu_commands_;
 void advance_gpu(port::driving::Memory&,float,const std::array<std::array<float,3>,3>&);
 void trigger_gpu(port::driving::Memory&,std::size_t,float,
     const std::array<std::array<float,3>,3>&,const std::array<float,16>&,
     const std::array<float,4>&,const std::array<float,16>*);
 void capture_spawn(const ParticleGroup&,std::size_t before);
 std::array<float,3> car_velocity_{};
 std::size_t static_count_=0;
 ParticleResources resources_;std::uint32_t seed_;std::vector<std::size_t> order_,definition_;std::list<ParticleGroup> groups_;
};
ParticleResources decode_particles(const std::vector<std::uint8_t>&);
}
