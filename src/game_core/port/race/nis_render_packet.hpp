#pragma once
// TASK-0112 E-0112-N01. Port immutable output carrier for source NIS actor draws.
#include "game_core/port/driving/memory.hpp"
#include <array>
#include <vector>
namespace port::race {
struct NisDrawPacket {
 std::uint32_t model_hash=0;
 std::array<float,16> world{};
 std::vector<std::array<float,16>> bones; //Actual ObjectToBoneSpace * posed matrix; empty for hand props.
};
// eAnimation.Render001A4230/4360 semantic draw transport, also component-testable
// without a live World. The supplied world is the already recovered actor matrix.
NisDrawPacket snapshot_source_animation(driving::Memory&,std::uint32_t animation,const std::array<float,16>& world);
// Call in native render publication, after source camera updates. This advances
// only source SetupForRendering presentation state; no World/rule/finish ticks.
// Renderer consumes copied packets, applies actual solid skin weights/culling.
std::vector<NisDrawPacket> snapshot_source_nis(driving::Memory&,std::uint32_t manager,unsigned view_id);
}
