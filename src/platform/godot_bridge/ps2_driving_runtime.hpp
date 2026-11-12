#pragma once
// TASK-0107. One real source World behind every user-facing driving entry.
#include "port/runtime/world_session.hpp"
#include "ps2_texture_loader.hpp"
#include <map>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/projection.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
namespace godot { class Node3D; class Camera3D; }
namespace port::driving {class WheelParticleEffects;}
namespace port::track {class ParticleSimulation;}
namespace port::godot_bridge {
struct WorldLoadJob;
class Ps2DrivingRuntime : public godot::RefCounted {
 GDCLASS(Ps2DrivingRuntime,godot::RefCounted)
 std::unique_ptr<port::race::WorldSession> world_;
 std::shared_ptr<WorldLoadJob> loading_;
 port::race::WorldLaunchRequest request_;
 std::uint64_t generation_=0,audio_step_=0;
 std::string phase_="Empty",error_;
 bool request_ready_=false;
 std::optional<port::race::WorldFrame> presented_;
 port::race::WorldSessionStats last_stats_;
 std::string last_cleanup_diagnostic_;
 godot::Ref<Ps2TextureLoader> skid_textures_;
 std::map<std::uint64_t,godot::Ref<godot::ShaderMaterial>> skid_materials_;
 void begin_load();void poll_load();void release_world();
protected:
 static void _bind_methods();
 virtual bool requests_enabled()const{return true;}
 void require_requests()const;
public:
 Ps2DrivingRuntime();~Ps2DrivingRuntime()override;
 static void shutdown_loader();
 const port::race::WorldFrame* presented_frame()const{return presented_?&*presented_:nullptr;}
 const port::race::WorldFrame* candidate_frame()const{return world_?&world_->frame():nullptr;}
 godot::Dictionary configure_world(int car_id,int skin_id,int track,bool reverse=false,const godot::String& car_code="");
 godot::Dictionary configure_event_world(const godot::Dictionary&);
 godot::Dictionary load_config(const godot::String&);
 godot::Dictionary set_track(int);
 godot::Dictionary action(const godot::String&);
 godot::Dictionary set_input(double,double,double);
 godot::Dictionary set_driving_input(double,double,double,double,int=-1,int=0,bool=false,bool=false);
 godot::Dictionary tick(double=0);
 godot::Dictionary tick_ui(double);
 godot::Dictionary menu_input(int,bool,int=0);
 godot::Dictionary acknowledge_event_attempt(const godot::Dictionary&);
 godot::Dictionary get_state()const;
 godot::Dictionary get_world_state()const;
 godot::Array get_world_route()const;
 godot::Dictionary get_world_road_course()const;
 godot::Array get_world_road_path()const;
 godot::PackedVector2Array take_world_pcm();
 godot::Dictionary commit_presentation(std::int64_t step,std::int64_t ui_revision=-1);
 std::shared_ptr<const port::track::ParticleSimulation> world_particle_snapshot()const;
 godot::Dictionary report_presentation_failure(const godot::String&);
 godot::Dictionary set_surface(int,int);
 godot::Dictionary update_driving_camera(double,bool=false);
 godot::Dictionary cycle_driving_camera_pov();
 godot::Dictionary get_driving_audio_bank()const;
 godot::Dictionary get_driving_audio_state()const;
 godot::Array take_collision_audio_events();
 godot::Array take_splash_audio_events();
 godot::Dictionary render_skids(const godot::Transform3D&,const godot::Projection&,double);
 godot::Dictionary present_particles(godot::Node3D*,godot::Camera3D*);
 void clear_particles();
 float particle_timestep(std::uint64_t)const;
 std::uint64_t particle_step()const;
 std::array<float,3> particle_velocity()const;
 void emit_wheel_particles(port::track::ParticleSimulation&,port::driving::WheelParticleEffects&,const std::array<std::array<float,3>,3>&);
};
}
