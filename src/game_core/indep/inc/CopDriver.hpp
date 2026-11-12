#pragma once
// TASK-0110 E-0110-CM01. Exact Debug declaration marker ../indep/inc/CopDriver.hpp;
// definition owner ../indep/src/copdriver.cpp, E-0042-S001. Logical-memory views,
// not a host PS2 ABI or asserted C++ inheritance model.
#include "game_core/port/driving/object_view.hpp"
#include <optional>
namespace port::race {struct PhysicsConstructionServices;}
namespace hp2::debug {
class CopMover:public ::port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void Sub_0019E650(std::uint32_t car,std::optional<std::uint32_t> ground_normal_w={});
 void Sub_0019E700(int flags);
 void ResetMaxThrottleAccel(); //0019E718.
 void DoSnapshot(std::uint32_t snapshot); //0019E798.
 float CalculateFacingAngle(float maximum_speed,float elapsed); //0019F448.
 float Move(float elapsed,::port::race::PhysicsConstructionServices&); //0019E8A8.
};
}
