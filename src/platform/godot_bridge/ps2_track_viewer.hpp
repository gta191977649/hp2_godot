#pragma once
#include "ps2_track_renderer.hpp"
namespace port::godot_bridge {
// Debug request policy only. Runtime and viewer use the exact same renderer.
class Ps2TrackViewer : public Ps2TrackRenderer {
    GDCLASS(Ps2TrackViewer,Ps2TrackRenderer)
protected:
    static void _bind_methods();
    bool requests_enabled() const override { return HP2_DEBUG_BUILD != 0; }
};
}
