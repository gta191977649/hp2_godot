#pragma once
#include <godot_cpp/variant/dictionary.hpp>
namespace port::godot_bridge {
// Engine-owned integration fixture; no original game function or public gameplay flow.
godot::Dictionary test_track_mesh_batch_cache();
}
