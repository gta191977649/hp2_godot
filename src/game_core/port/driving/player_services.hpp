#pragma once
// TASK-0046: explicit tool environment for original RealDriver calls. No PS2 ABI.
#include <functional>
#include "memory.hpp"
namespace port::driving {
struct PlayerServices {
    std::function<void(Memory&,std::uint32_t)> MaybeAddTrackRoutePoint;
    std::function<void(Memory&,std::uint32_t)> RealDriverAddNearbyDebugTopology;
    std::function<bool(std::int32_t)> IsJoystickPluggedIn;
    std::function<bool(std::int32_t)> IsJoystickTypeWheel;
    std::function<void(Memory&,float,const char*)> RegisterInterestingEvent;
};
}
