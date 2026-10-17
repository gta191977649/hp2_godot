#pragma once
// Shared runtime host for original track frame commands; no host physics.
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <memory>
namespace port::godot_bridge {
class Ps2TrackRenderer : public godot::Node3D {
    GDCLASS(Ps2TrackRenderer,godot::Node3D)
    struct Impl;
    std::unique_ptr<Impl> impl_;
    void release_display();
    void build_debug_display();
    void apply_debug_display();
protected:
    static void _bind_methods();
    virtual bool requests_enabled() const { return true; }
public:
    Ps2TrackRenderer();
    ~Ps2TrackRenderer() override;
    godot::Array get_catalog();
    godot::Dictionary request_track(int number);
    godot::Dictionary load_track(const godot::String& relative_path);
    godot::Dictionary pump_load();
    godot::Dictionary get_state() const;
    godot::Dictionary frame_summary() const {return get_state();}
    void clear_track();
    void set_render_collision(bool enabled);
    void set_render_material(bool enabled);
    godot::Dictionary update_view(const godot::Vector3& camera);
};
}
