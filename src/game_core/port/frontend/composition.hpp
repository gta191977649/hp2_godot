#pragma once
// TASK-0098 E-0098-F04. Port keys for the original Debug shared alpha domain.
// This is command preparation; it does not supply a Godot/GS rendering backend.
#include <cstddef>
#include <cstdint>
#include <vector>
namespace port::driving {class Memory;}
namespace port::selection {
struct FlailerDepth {float gs_depth=0,alpha_sort_depth=0;std::uint32_t packed_depth=0;};
// Debug0010FE48 Flailer-only depth slice. z_max is the resolved target's
//16/24-bit ZMax, never inferred from a Godot camera or draw order.
FlailerDepth flailer_depth_0010FE48(const port::driving::Memory&,int layer,float z_max);
struct AlphaKey {std::int8_t rendering_order=0;float depth=0;};
// Input is original linked-list traversal order. Output is the rebuilt list's
// order after Debug RadixSortAlphaKeys00104FE0 / SortAlphaKeys001053B8.
std::vector<std::size_t> alpha_order_001053B8(const std::vector<AlphaKey>&);
}
