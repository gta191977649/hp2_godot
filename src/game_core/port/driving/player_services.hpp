#pragma once
// Port primitive joystick environment for original RealDriver calls. No PS2 ABI.
#include <functional>
#include "memory.hpp"
namespace port::driving {
struct PlayerServices {
    std::function<bool(std::int32_t)> IsJoystickPluggedIn;
    std::function<bool(std::int32_t)> IsJoystickTypeWheel;
};
}
// Port declarations for exact original game helpers, not injectable services.
namespace hp2::debug {
void MaybeAddTrackRoutePoint(::port::driving::Memory&,std::uint32_t position,std::uint32_t velocity,float elevation,int direction,int car_class); //001C2ED0, E-0099-W63.
void RealDriverAddNearbyDebugTopology(::port::driving::Memory&,std::uint32_t topology,std::uint32_t position); //001FEE80, exact original empty target.
}
