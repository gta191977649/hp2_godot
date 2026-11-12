#pragma once
// TASK-0099 E-0099-W71. Original ../indep/inc/Clans.hpp spelling from Debug
// inline markers; port Memory views, no original host ABI assertion.
#include "game_core/port/driving/object_view.hpp"
namespace hp2::debug {
class Clan:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 void Sub_001A8F90(std::uint32_t position,std::uint32_t hash);
 void Sub_001A8FE0(int flags);
 void ExpandBoundingBox(std::uint32_t bbox_min,std::uint32_t bbox_max); //001A8B90.
};
std::uint32_t GetClan(::port::driving::Memory&,std::uint32_t position); //001A8D50.
void FlushClans(::port::driving::Memory&); //001A8CD0.
}
