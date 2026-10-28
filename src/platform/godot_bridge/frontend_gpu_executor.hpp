#pragma once
// Port device backend. One colour/depth framebuffer for all native 2D/3D stages.
// Local RD plus readback is an explicit initial transport, not a performance claim.
#include "port/render/frontend_packet.hpp"
#include <godot_cpp/classes/image.hpp>
#include <memory>
namespace port::godot_bridge {
class FrontendGpuExecutor {
    struct Impl;std::unique_ptr<Impl> p_;
public:
    FrontendGpuExecutor();~FrontendGpuExecutor();
    godot::Ref<godot::Image> render(const port::render::FrontendPacket&);
};
}
