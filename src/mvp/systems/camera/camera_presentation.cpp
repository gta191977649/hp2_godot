#include "camera_presentation.hpp"

#include "assets/mesh_converter.hpp"
#include "bridge/presentation_snapshot.hpp"

#include <godot_cpp/classes/camera3d.hpp>

#include <stdexcept>

namespace hp2 {

void apply_race_camera(godot::Camera3D& camera, const RaceCameraState& state) {
    const auto eye = race_vector(state.eye.data());
    const auto forward = race_vector(state.forward.data());
    // Source view raster is Y-down. Godot's Camera3D up is Y-up.
    const auto up = -race_vector(state.up.data());
    if (forward.cross(up).length_squared() < 0.01f)
        throw std::runtime_error("Primary camera has a degenerate basis");
    camera.set_perspective(state.verticalFov, state.nearClip, state.farClip);
    camera.look_at_from_position(eye, eye + forward, up);
}

}
