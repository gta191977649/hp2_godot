#pragma once
#include "game_core/port/driving/session.hpp"
#include "ps2_texture_loader.hpp"
#include <godot_cpp/classes/audio_stream_wav.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/variant/projection.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/dictionary.hpp>
namespace godot {class Node3D;class Camera3D;}
namespace port::driving {class NativeBackend;class WheelParticleEffects;struct VehicleSoundBank;class EngineAudioMixer;struct EngineAudioFrame;struct CollisionSoundBank;struct TireSoundBank;struct SplashSoundBank;class DrivingCamera;}
namespace port::track {class ParticleSimulation;}
namespace port::godot_bridge {
class DrivingParticleDisplay;
// Shared port-side driving boundary. Original physics remains in the native core.
class Ps2DrivingRuntime : public godot::RefCounted {
    GDCLASS(Ps2DrivingRuntime,godot::RefCounted)
    port::driving::Session session_;
    std::shared_ptr<port::driving::NativeBackend> backend_;
    std::string code_;
    godot::Ref<Ps2TextureLoader> skid_textures_;
    std::map<std::uint64_t,godot::Ref<godot::ShaderMaterial>> skid_materials_;
    std::unique_ptr<DrivingParticleDisplay> particle_display_;
    std::shared_ptr<port::driving::VehicleSoundBank> sound_bank_;
    std::vector<godot::Ref<godot::AudioStreamWAV>> audio_streams_;
    std::shared_ptr<port::driving::EngineAudioMixer> audio_mixer_;
    std::shared_ptr<port::driving::EngineAudioFrame> audio_frame_;
    std::uint64_t audio_step_=0;
    std::shared_ptr<port::driving::CollisionSoundBank> collision_bank_;
    std::vector<godot::Ref<godot::AudioStreamWAV>> collision_streams_;
    godot::Array collision_audio_events_;
    std::shared_ptr<port::driving::SplashSoundBank> splash_bank_;
    std::vector<godot::Ref<godot::AudioStreamWAV>> splash_streams_;
    godot::Array splash_audio_events_;
    std::shared_ptr<port::driving::TireSoundBank> tire_bank_;
    std::vector<godot::Ref<godot::AudioStreamWAV>> tire_streams_;
    std::shared_ptr<port::driving::DrivingCamera> driving_camera_;
    std::optional<port::driving::Config> default_camera_config_;
    void update_audio();
    void reset_audio();
protected:
    static void _bind_methods();
    virtual bool requests_enabled() const { return true; }
    void require_requests() const;
public:
    Ps2DrivingRuntime();
    ~Ps2DrivingRuntime() override;
    godot::Dictionary present_particles(godot::Node3D*,godot::Camera3D*);
    void clear_particles();
    // C++ visual seam: shared WorldEffects storage, never a second physics step.
    float particle_timestep(std::uint64_t last_step)const;
    std::uint64_t particle_step()const{return session_.snapshot().step;}
    std::array<float,3> particle_velocity()const{return session_.snapshot().velocity.value_or(std::array<float,3>{});}
    void emit_wheel_particles(port::track::ParticleSimulation&,port::driving::WheelParticleEffects&,const std::array<std::array<float,3>,3>&);
    godot::Dictionary load_config(const godot::String& code);
    godot::Dictionary action(const godot::String& name);
    godot::Dictionary set_input(double throttle,double brake,double steering);
    godot::Dictionary set_driving_input(double throttle,double brake,double steering,double handbrake,int transmission=-1,int shift=0,bool neutral=false,bool horn=false);
    godot::Dictionary set_surface(int mode,int terrain);
    godot::Dictionary set_track(int number);
    godot::Dictionary tick(double elapsed=0);
    godot::Dictionary get_state()const;
    godot::Dictionary update_driving_camera(double elapsed,bool look_back=false);
    godot::Dictionary cycle_driving_camera_pov();
    godot::Dictionary get_driving_audio_bank()const;
    godot::Dictionary get_driving_audio_state()const;
    godot::Array take_collision_audio_events();
    godot::Array take_splash_audio_events();
    godot::Dictionary render_skids(const godot::Transform3D& camera,const godot::Projection& clip,double H);
};
}
