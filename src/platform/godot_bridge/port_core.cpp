#include "port_core.hpp"
#include "game_core/scaffold.hpp"
#include <godot_cpp/core/class_db.hpp>

namespace port::godot_bridge {
void PortCore::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("tick", "axis"), &PortCore::tick);
    godot::ClassDB::bind_method(godot::D_METHOD("frame"), &PortCore::frame);
    godot::ClassDB::bind_method(godot::D_METHOD("scaffold_add", "left", "right"), &PortCore::scaffold_add);
}
std::int64_t PortCore::tick(int axis) { core::demo_tick(demo, axis); return demo.position; }
std::int64_t PortCore::frame() const { return static_cast<std::int64_t>(demo.frame); }
std::int64_t PortCore::scaffold_add(std::int32_t left, std::int32_t right) const {
    return core::scaffold_add(left, right);
}
}
