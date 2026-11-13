#pragma once

#include <cstddef>

struct Scenery;
namespace godot { class Node3D; }

namespace hp2 {

class SkyResources;
std::size_t attach_track(godot::Node3D& root, const Scenery& scenery, const SkyResources* sky);

}
