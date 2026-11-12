#pragma once
// TASK-0112 E-0112-NIS03; native companion views, no host PS2 ABI assertion.
#include "game_core/port/driving/object_view.hpp"
#include <array>
namespace hp2::debug {
class PreviewRoute:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 void Sub_00163FF8(std::uint32_t car,float tolerance,float distance);
 void Sub_00164208(int);
 float NextPoint(int,float,int); //00164260.
 float ClosestDistance(std::uint32_t preview,std::uint32_t route,std::uint32_t point,const std::array<float,2>&,int); //00164460.
 bool ClosestPoint(const std::array<float,2>&,float,int,int,std::uint32_t); //00164640.
};
void TransformBoundingBox(::port::driving::Memory&,std::array<float,4>&,std::array<float,4>&,const std::array<float,16>&); //001AECA0.
bool AnyWorldObjectsInBox(::port::driving::Memory&,const std::array<float,4>&,const std::array<float,4>&); //001CAB50.
}
