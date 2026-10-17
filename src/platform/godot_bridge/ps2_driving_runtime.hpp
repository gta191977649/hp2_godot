#pragma once
#include "game_core/port/driving/session.hpp"
#include "ps2_texture_loader.hpp"
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/variant/projection.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/dictionary.hpp>
namespace port::driving {class NativeBackend;}
namespace port::godot_bridge {
// Shared port-side driving boundary. Original physics remains in the native core.
class Ps2DrivingRuntime : public godot::RefCounted {
    GDCLASS(Ps2DrivingRuntime,godot::RefCounted)
    port::driving::Session session_;
    std::shared_ptr<port::driving::NativeBackend> backend_;
    std::string code_;
    godot::Ref<Ps2TextureLoader> skid_textures_;
    std::map<std::uint64_t,godot::Ref<godot::ShaderMaterial>> skid_materials_;
protected:
    static void _bind_methods();
    virtual bool requests_enabled() const { return true; }
    void require_requests() const;
public:
    godot::Dictionary load_config(const godot::String& code);
    godot::Dictionary action(const godot::String& name);
    godot::Dictionary set_input(double throttle,double brake,double steering);
    godot::Dictionary set_driving_input(double throttle,double brake,double steering,double handbrake,int transmission=-1,int shift=0,bool neutral=false);
    godot::Dictionary set_surface(int mode,int terrain);
    godot::Dictionary set_track(int number);
    godot::Dictionary tick(double elapsed=0);
    godot::Dictionary get_state()const;
    godot::Dictionary render_skids(const godot::Transform3D& camera,const godot::Projection& clip,double H);
};
}
