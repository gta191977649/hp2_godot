#pragma once
#include "ps2_driving_runtime.hpp"
namespace port::godot_bridge {
// Debug entry only; the runtime boundary owns the native Session and all behavior.
class Ps2DrivingLab : public Ps2DrivingRuntime {
    GDCLASS(Ps2DrivingLab, Ps2DrivingRuntime)
protected:
    static void _bind_methods();
    bool requests_enabled() const override { return HP2_DEBUG_BUILD != 0; }
};
}
