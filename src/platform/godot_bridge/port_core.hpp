#pragma once
#include <cstdint>
#include "game_core/demo.hpp"
#include <godot_cpp/classes/ref_counted.hpp>

namespace port::godot_bridge {
class PortCore : public godot::RefCounted {
    GDCLASS(PortCore, godot::RefCounted)
protected:
    static void _bind_methods();
public:
    core::DemoState demo;
    std::int64_t tick(int axis);
    std::int64_t frame() const;
    std::int64_t scaffold_add(std::int32_t left, std::int32_t right) const;
};
}
