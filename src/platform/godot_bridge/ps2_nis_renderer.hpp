#pragma once
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <memory>
namespace port::godot_bridge {
// Host display of copied original NIS draws. No game Memory or update callbacks.
class Ps2NisRenderer:public godot::Node3D {
 GDCLASS(Ps2NisRenderer,godot::Node3D)
 struct Impl;std::unique_ptr<Impl> impl_;
protected:static void _bind_methods();
public:Ps2NisRenderer();~Ps2NisRenderer() override;
 godot::Dictionary apply_packets(const godot::Array&);
};
}
