#pragma once
#include "ps2_vehicle_renderer.hpp"
namespace port::godot_bridge {
// Debug entry only. All loading/rendering/material work is inherited from the runtime backend.
class Ps2CarViewer : public Ps2VehicleRenderer {
    GDCLASS(Ps2CarViewer, Ps2VehicleRenderer)
protected:
    static void _bind_methods();
    bool requests_enabled() const override { return HP2_DEBUG_BUILD != 0; }
};
}
