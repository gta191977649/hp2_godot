#pragma once
// Shared runtime host for original track frame commands; no host physics.
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <memory>
namespace godot { class Camera3D; }
namespace godot { class ArrayMesh; template<class T> class Ref; }
namespace port::track { struct Frame; }
namespace port::godot_bridge {
class Ps2DrivingRuntime;
class Ps2TrackRenderer : public godot::Node3D {
    GDCLASS(Ps2TrackRenderer,godot::Node3D)
    struct Impl;
    std::unique_ptr<Impl> impl_;
    void release_display();
    void trim_draw_cache();
    void release_debug_display();
    godot::Ref<godot::ArrayMesh> acquire_source_mesh(std::size_t,std::size_t);
    void build_debug_display();
    void apply_debug_display();
    void submit_frame(port::track::Frame&,godot::Camera3D*,unsigned view_id);
    void complete_visual_readback();
    void depth_source_structure_changed(godot::Node*);
    void depth_source_order_changed();
protected:
    static void _bind_methods();
    void _notification(int what);
    virtual bool requests_enabled() const { return true; }
public:
    Ps2TrackRenderer();
    ~Ps2TrackRenderer() override;
    godot::Array get_catalog();
    godot::Dictionary request_track(int number);
    godot::Dictionary load_track(const godot::String& relative_path);
    godot::Dictionary pump_load();
    godot::Dictionary get_state() const;
    godot::Dictionary get_memory_state() const;
    godot::Dictionary frame_summary() const;
    void clear_track();
    godot::Dictionary configure_optimizations(const godot::Dictionary&);
    godot::Dictionary get_optimization_state()const;
    godot::Dictionary configure_lod_backend(const godot::String&);
    godot::Dictionary get_lod_state()const;
    godot::Dictionary get_lod_ranges(int instance,int view)const;
    void set_effect_enabled(const godot::String& id,bool enabled);
    godot::Array get_effects() const;
    void restore_render_defaults();
    void set_diagnostics_level(const godot::String& level);
    void set_render_props(bool enabled);
    void set_visibility_override(bool enabled,double distance);
    godot::Dictionary get_render_controls() const;
    godot::Dictionary get_environment_capture() const;
    godot::Dictionary get_render_targets() const;
    void set_camera_mode(int original_type);
    void set_visual_timestep(double seconds);
    void set_visual_state(const godot::Dictionary&);
    void set_driving_effect_source(const godot::Ref<Ps2DrivingRuntime>&);
    void set_render_collision(bool enabled);
    void set_render_material(bool enabled);
    void set_show_lod(bool enabled);
    godot::Dictionary update_view(const godot::Vector3& camera);
};
}
