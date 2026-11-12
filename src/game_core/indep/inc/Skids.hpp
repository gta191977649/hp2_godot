#pragma once
// TASK-0099 E-0099-W71. Actual shared original pools/list ownership. Names from
// Debug ../indep/inc/Skids.hpp; address parameters denote original Memory.
#include "game_core/port/driving/object_view.hpp"
#include <optional>
namespace hp2::debug {
void Sub_001A8B30(::port::driving::Memory&); //original skid-list process static.
class SkidSet:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 void Sub_001A8008(std::uint32_t maker,std::uint32_t car,std::uint32_t position,std::uint32_t delta,int terrain,float intensity,std::optional<std::uint32_t> radius_stack_w);
 void Sub_001A80D8(int flags);
 int AddSegment(std::uint32_t position,std::uint32_t delta,float intensity,std::optional<std::uint32_t> radius_stack_w); //001A8150.
 void FinishedAddingSkids(); //001A8398.
 void GetLastPoints(std::uint32_t position,std::uint32_t delta); //001A8B68; XYZ-only stores.
};
class SkidMaker:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 void MakeSkid(std::uint32_t car,std::uint32_t position,std::uint32_t delta,int terrain,float intensity,std::optional<std::uint32_t> radius_stack_w,std::optional<std::uint32_t> restart_position_stack_w); //001A8648.
 void MakeNoSkid(); //001A8778.
};
std::uint32_t CreateNewSkidSet(::port::driving::Memory&,std::uint32_t maker,std::uint32_t car,std::uint32_t position,std::uint32_t delta,int terrain,float intensity,std::optional<std::uint32_t> radius_stack_w); //001A8578.
void DeleteThisSkid(::port::driving::Memory&,std::uint32_t set); //001A88A8.
}
