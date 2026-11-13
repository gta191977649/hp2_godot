#pragma once

namespace godot { class Camera3D; }

namespace hp2 {

struct RaceCameraState;
void apply_race_camera(godot::Camera3D& camera, const RaceCameraState& state);

}
