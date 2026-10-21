#pragma once
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <memory>

namespace port::godot_bridge {
// Shared Godot backend for native CarRenderInfo/vehicle render data.
// Usable by runtime scenes and debug viewers; recovered logic remains Godot-free.
class Ps2VehicleRenderer : public godot::Node3D {
    GDCLASS(Ps2VehicleRenderer, godot::Node3D)
    struct Impl;
    std::unique_ptr<Impl> impl_;
    void release_display();
    void rebuild_display(bool incremental=false);
    void layout_lods();
    void cancel_load();
    godot::Dictionary load_requested(const godot::String& path, int car_id, int skin_id);
protected:
    static void _bind_methods();
    virtual bool requests_enabled() const { return true; }
public:
    static void shutdown_loader();
    Ps2VehicleRenderer();
    ~Ps2VehicleRenderer() override;
    godot::Dictionary request_car(const godot::String& relative_path,int car_id,int skin_id);
    godot::Dictionary pump_load();
    godot::Dictionary set_show_lods(bool enabled);
    godot::Dictionary set_wheel_blur(bool enabled);
    godot::Dictionary set_wheel_speed(double radians_per_second);
    void advance_wheels(double delta);
    godot::Dictionary apply_driving_snapshot(const godot::Dictionary& snapshot);
    godot::Dictionary load_car(const godot::String& relative_path);
    godot::Array get_catalog();
    godot::Dictionary load_car_id(int car_id, int skin_id);
    godot::Dictionary set_skin(int skin_id);
    void set_inspection(bool enabled);
    void clear_car();
    godot::Dictionary update_view(const godot::Vector3& position, const godot::Vector3& forward,
                                  double projection_scale, int forced_lod);
    godot::Dictionary get_state() const;
    godot::Dictionary get_visual_state() const;
    void set_environment_capture(const godot::Dictionary& capture);
    godot::Ref<godot::Texture2D> get_visual_texture(std::int64_t hash) const;
    void set_part_visible(const godot::String& name, bool visible);
    void set_wireframe(bool enabled);
    void set_bounds(bool enabled);
};
}
