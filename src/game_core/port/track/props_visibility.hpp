#pragma once
#include "runtime.hpp"
#include <algorithm>
#include <unordered_set>
namespace port::track {
// Port-side presentation policy: placed/transformed scenery is hidden, while
// world-identity scenery (roads and baked world geometry) remains. This is not
// an original-game visibility flag or an inferred model-name taxonomy.
inline void apply_props_visibility(Frame& frame,bool enabled){
    if(enabled)return;
    frame.commands.erase(std::remove_if(frame.commands.begin(),frame.commands.end(),
        [](const DrawCommand& command){return !command.identity;}),frame.commands.end());
    std::unordered_set<std::size_t> visible;
    for(const auto& command:frame.commands)visible.insert(command.instance);
    frame.visible_instances=visible.size();
}
}
