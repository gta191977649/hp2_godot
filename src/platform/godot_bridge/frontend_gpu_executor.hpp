#pragma once
// Port device backend. One colour/depth framebuffer for all native 2D/3D stages.
// TASK-0105: main-device work runs on the render thread, without image readback.
#include "port/render/frontend_packet.hpp"
#include <godot_cpp/classes/texture2drd.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <memory>
namespace port::godot_bridge {
class FrontendGpuExecutor {
    struct Impl;std::shared_ptr<Impl> p_;
    struct Work;
    static void execute(std::int64_t work);
public:
    struct ExternalTexture {port::render::TextureId id;godot::Ref<godot::Texture2D> texture;};
    FrontendGpuExecutor();~FrontendGpuExecutor();
    godot::Ref<godot::Texture2DRD> render(port::render::FrontendPacket,std::vector<ExternalTexture> external={});
    godot::Dictionary statistics()const;
};
}
